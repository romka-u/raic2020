#include "MyStrategy.hpp"
#include <exception>

#include "world.h"
#include "actions.h"
#include "astar.h"

World world;
GameStatus gameStatus;
vector<pair<Cell, Cell>> debugTargets;

MyStrategy::MyStrategy() {}

Action MyStrategy::getAction(const PlayerView& playerView, DebugInterface* debugInterface)
{
    cerr << "=================== Tick " << playerView.currentTick << "======================\n";
    if (props.empty()) props = playerView.entityProperties;
    world.update(playerView);
    int resourcesLeft = playerView.players[playerView.myId - 1].resource;
    cerr << "my resources: " << resourcesLeft << endl;
    int myId = playerView.myId;
    std::unordered_map<int, EntityAction> moves;
    memset(busyForAStar, 0, sizeof(busyForAStar));

    gameStatus.update(playerView, world);

    calcSquadsTactic(myId, world, gameStatus);

    vector<MyAction> actions;
    addBuildActions(playerView, world, actions, gameStatus);
    addTrainActions(myId, world, actions, gameStatus);
    addRepairActions(myId, world, actions, gameStatus);
    addWarActions(playerView, world, actions, gameStatus);
    addGatherActions(myId, world, actions, gameStatus);

    sort(actions.begin(), actions.end());

    unordered_set<int> usedUnits, usedResources;
    int bestRepairScore = -1;
    debugTargets.clear();

    auto setMove = [&moves](int unitId, const Cell& from, const Cell& to) {
        // Cell nc = getNextCellTo(world, from, to);
        // if (world.eMap[nc.x][nc.y] != 0) nc = to;
        moves[unitId].moveAction = std::make_shared<MoveAction>(to, true, false);
    };

    for (const MyAction& action : actions) {
        auto [unitId, actionType, pos, oid, score] = action;
        const auto& upos = world.entityMap[unitId].position;
        const EntityType etype = static_cast<EntityType>(oid);

        if (usedUnits.find(unitId) != usedUnits.end()) continue;

        switch (actionType) {
            case A_ATTACK:
                moves[unitId].attackAction = std::make_shared<AttackAction>(std::make_shared<int>(oid), nullptr);
                // cerr << unitId << " at " << upos << " wants to attack " << oid << " at " << world.entityMap[oid].position << endl;
                break;
            case A_GATHER:
                if (usedResources.insert(oid).second) {
                    if (dist(upos, pos) == 1) {
                        moves[unitId].attackAction = std::make_shared<AttackAction>(std::make_shared<int>(oid), nullptr);
                        // cerr << unitId << " at " << upos << " gathers at " << world.entityMap[oid].position << endl;
                    } else {
                        setMove(unitId, upos, pos);
                        // cerr << unitId << " at " << upos << " moves to gather at " << pos  << ", result by astar is " << moves[unitId].moveAction->target << endl;
                    }
                } else {
                    continue;
                }
                break;
            case A_MOVE:
                setMove(unitId, upos, pos);
                // cerr << unitId << " at " << upos << " wants to move to " << pos << ", result by astar is " << moves[unitId].moveAction->target << endl;
                break;
            case A_BUILD:
            case A_TRAIN:
                if (props.at(etype).cost <= resourcesLeft) {
                    moves[unitId].buildAction = std::make_shared<BuildAction>(etype, pos);
                    // cerr << unitId << " at " << upos << " wants to build/train [" << oid << "] at " << pos << endl;
                    resourcesLeft -= props.at(etype).cost;
                } else {
                    continue;
                }
                break;
            case A_REPAIR:
                moves[unitId].repairAction = std::make_shared<RepairAction>(oid);
                // cerr << unitId << " at " << upos << " wants to repair " << oid << endl;
                break;
            case A_REPAIR_MOVE:
                if (bestRepairScore == -1 || score.main == 199) {
                    setMove(unitId, upos, pos);
                    bestRepairScore = score.main;
                    cerr << unitId << " at " << upos << " wants to repair_move to " << pos << "( score.main = " << score.main << ")" << endl;
                } else {
                    continue;
                }
                break;
            default:
                assert(false);
        }

        usedUnits.insert(unitId);
        Cell t = upos;
        busyForAStar[t.x][t.y] = true;
        if (moves[unitId].moveAction) {
            t = moves[unitId].moveAction->target;
            busyForAStar[t.x][t.y] = true;
            debugTargets.emplace_back(upos, t);
        }
    }   

    return Action(moves);
}

void MyStrategy::debugUpdate(const PlayerView& playerView, DebugInterface& debugInterface)
{
    debugInterface.send(DebugCommand::Clear());
    vector<ColoredVertex> vertices;
    Vec2Float empty(0, 0);

    static vector<Color> colors = {
        Color(0, 0, 0, 0.25),
        Color(1.0, 0, 0, 0.5),
        Color(0, 0, 1.0, 0.5),
        Color(0, 0.8, 0.8, 0.5),
        Color(0.8, 0, 0.8, 0.5)
    };

    forn(x, 80) forn(y, 80) {
        bool aside = false;
        const int val = world.infMap[x][y];
        forn(w, 4) {
            Cell c{x + dx[w], y + dy[w]};
            if (c.inside() && val != world.infMap[c.x][c.y])
                aside = true;
        }

        if (aside) {
            vertices.push_back(ColoredVertex(make_shared<Vec2Float>(x + 0.3, y + 0.5), empty, colors[val]));
            vertices.push_back(ColoredVertex(make_shared<Vec2Float>(x + 0.7, y + 0.5), empty, colors[val]));
            vertices.push_back(ColoredVertex(make_shared<Vec2Float>(x + 0.5, y + 0.3), empty, colors[val]));
            vertices.push_back(ColoredVertex(make_shared<Vec2Float>(x + 0.3, y + 0.5), empty, colors[val]));
            vertices.push_back(ColoredVertex(make_shared<Vec2Float>(x + 0.7, y + 0.5), empty, colors[val]));
            vertices.push_back(ColoredVertex(make_shared<Vec2Float>(x + 0.5, y + 0.7), empty, colors[val]));
        }
    }

    debugInterface.send(DebugCommand::Add(std::make_shared<DebugData::Primitives>(vertices, PrimitiveType::TRIANGLES)));

    vector<ColoredVertex> lines;
    Color targetColor(0, 0, 0, 0.9);
    for (const auto& [from, to] : debugTargets) {
        lines.push_back(ColoredVertex(make_shared<Vec2Float>(from.x + 0.5, from.y + 0.5), empty, targetColor));
        lines.push_back(ColoredVertex(make_shared<Vec2Float>(to.x + 0.5, to.y + 0.5), empty, targetColor));
    }
    debugInterface.send(DebugCommand::Add(std::make_shared<DebugData::Primitives>(lines, PrimitiveType::LINES)));

    debugInterface.getState();
}