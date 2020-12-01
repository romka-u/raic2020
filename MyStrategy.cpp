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
    int myId = playerView.myId;
    std::unordered_map<int, EntityAction> moves;
    clearAStar();

    if (world.oppEntities.empty()) return Action();

    gameStatus.update(playerView, world);

    calcSquadsTactic(myId, world, gameStatus);

    vector<MyAction> actions;
    addBuildActions(playerView, world, actions, gameStatus);
    addTrainActions(playerView, world, actions, gameStatus);
    addRepairActions(myId, world, actions, gameStatus);
    addTurretsActions(playerView, world, actions, gameStatus);
    addWarActions(playerView, world, actions, gameStatus);
    addGatherActions(myId, world, actions, gameStatus);

    sort(actions.begin(), actions.end());

    unordered_set<int> usedUnits, usedResources, hiding;
    int bestRepairScore = -1;
    debugTargets.clear();
    vector<pair<pair<int, int>, pair<Cell, Cell>>> fmoves;

    auto setMove = [&fmoves](int unitId, const Cell& from, const Cell& to) {
        fmoves.emplace_back(make_pair(dist(from, to), unitId), make_pair(from, to));
    };

    gameStatus.workersLeftToFixTurrets = false;
    for (int bi : world.buildings[myId]) {
        const auto& bu = world.entityMap.at(bi);
        if (bu.entityType == EntityType::MELEE_BASE || bu.entityType == EntityType::BUILDER_BASE || bu.entityType == EntityType::RANGED_BASE) {
            moves[bi].buildAction = shared_ptr<BuildAction>();
        }
    }

    for (const MyAction& action : actions) {
        auto [unitId, actionType, pos, oid, score] = action;
        const auto& upos = world.entityMap[unitId].position;
        const EntityType etype = static_cast<EntityType>(oid);

        if (usedUnits.find(unitId) != usedUnits.end()) continue;
        bool moved = false;

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
                        moved = true;
                        // cerr << unitId << " at " << upos << " moves to gather at " << pos  << ", result by astar is " << moves[unitId].moveAction->target << endl;
                    }
                } else {
                    continue;
                }
                break;
            case A_MOVE:
                setMove(unitId, upos, pos);
                moved = true;
                // cerr << unitId << " at " << upos << " wants to move to " << pos << ", result by astar is " << moves[unitId].moveAction->target << endl;
                break;
            case A_HIDE_MOVE:
                hiding.insert(unitId);
                setMove(unitId, upos, pos);
                moved = true;
                // cerr << unitId << " at " << upos << " wants to hide to " << pos << ", result by astar is " << moves[unitId].moveAction->target << endl;
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
                bestRepairScore = 200;
                // cerr << unitId << " at " << upos << " wants to repair " << oid << endl;
                break;
            case A_REPAIR_MOVE:
                if (bestRepairScore == -1 || score.main >= 199 || score.main <= -2000) {
                    if (score.main == -3000) gameStatus.workersLeftToFixTurrets = true;
                    setMove(unitId, upos, pos);
                    moved = true;
                    bestRepairScore = score.main;
                    // cerr << unitId << " at " << upos << " wants to repair_move to " << pos << "( score.main = " << score.main << ")" << endl;
                } else {
                    continue;
                }
                break;
            default:
                assert(false);
        }

        if (!moved) {
            updateAStar(world, {upos});
        }

        usedUnits.insert(unitId);
        /*if (moves[unitId].moveAction) {
            Cell t = moves[unitId].moveAction->target;
            debugTargets.emplace_back(upos, t);
        }*/
    }

    sort(fmoves.begin(), fmoves.end());

    for (const auto [fi, se] : fmoves) {
        const int unitId = fi.second;
        const auto [from, to] = se;
        debugTargets.emplace_back(from, to);
        /*
        // cerr << "path from " << from << " to " << to << ":"; cerr.flush();
        vector<Cell> path = getPathTo(world, from, to);        
        // for (const Cell& c : path) cerr << " " << c;
        // cerr << endl;
        updateAStar(world, path);
        Cell target = to;
        
        if (path.size() >= 2) {
            target = path[1];
            // for (size_t i = 1; i < path.size(); i++)
            //     debugTargets.emplace_back(path[i-1], path[i]);
        }
        */
        // cerr << "set move target for " << from << "->" << to << " : " << target << endl;
        moves[unitId].moveAction = std::make_shared<MoveAction>(to, true, false);
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
        Color(0, 0, 1.0, 0.5),
        Color(0, 1.0, 0, 0.5),
        Color(1.0, 0, 0, 0.5),
        Color(0, 1.0, 1.0, 0.5)
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