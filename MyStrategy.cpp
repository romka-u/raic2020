#include "MyStrategy.hpp"
#include <exception>

#include "world.h"
#include "actions.h"

World world;
GameStatus gameStatus;

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

    gameStatus.update(playerView, world);

    calcSquadsTactic(myId, world, gameStatus);

    vector<MyAction> actions;
    addBuildActions(playerView, world, actions, gameStatus);
    addTrainActions(myId, world, actions, gameStatus);
    addRepairActions(myId, world, actions, gameStatus);
    addWarActions(myId, world, actions, gameStatus);
    addGatherActions(myId, world, actions, gameStatus);

    sort(actions.begin(), actions.end());

    unordered_set<int> usedUnits, usedResources;
    int bestRepairScore = -1;

    for (const MyAction& action : actions) {
        auto [unitId, actionType, pos, oid, score] = action;
        const auto& upos = world.entityMap[unitId].position;
        const EntityType etype = static_cast<EntityType>(oid);

        if (usedUnits.find(unitId) != usedUnits.end()) continue;

        switch (actionType) {
            case A_ATTACK:
                moves[unitId].attackAction = std::make_shared<AttackAction>(std::make_shared<int>(oid), nullptr);
                cerr << unitId << " at " << upos << " wants to attack " << oid << " at " << world.entityMap[oid].position << endl;
                break;
            case A_GATHER:
                if (usedResources.insert(oid).second) {
                    if (dist(upos, pos) == 1) {
                        moves[unitId].attackAction = std::make_shared<AttackAction>(std::make_shared<int>(oid), nullptr);
                        // cerr << unitId << " at " << upos << " gathers at " << world.entityMap[oid].position << endl;
                    } else {
                        moves[unitId].moveAction = std::make_shared<MoveAction>(pos, true, false);
                        // cerr << unitId << " at " << upos << " moves to gather at " << pos << endl;
                    }
                } else {
                    continue;
                }
                break;
            case A_MOVE:
                moves[unitId].moveAction = std::make_shared<MoveAction>(pos, true, false);
                // cerr << unitId << " at " << upos << " wants to move to " << pos << endl;
                break;
            case A_BUILD:
            case A_TRAIN:
                if (props.at(etype).cost <= resourcesLeft) {
                    moves[unitId].buildAction = std::make_shared<BuildAction>(etype, pos);
                    cerr << unitId << " at " << upos << " wants to build/train [" << oid << "] at " << pos << endl;
                    resourcesLeft -= props.at(etype).cost;
                } else {
                    continue;
                }
                break;
            case A_REPAIR:
                moves[unitId].repairAction = std::make_shared<RepairAction>(oid);
                cerr << unitId << " at " << upos << " wants to repair " << oid << endl;
                break;
            case A_REPAIR_MOVE:
                if (bestRepairScore == -1 || score.main == 199) {
                    moves[unitId].moveAction = std::make_shared<MoveAction>(pos, true, false);
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
    }   

    return Action(moves);
}

void MyStrategy::debugUpdate(const PlayerView& playerView, DebugInterface& debugInterface)
{
    debugInterface.send(DebugCommand::Clear());
    debugInterface.getState();
}