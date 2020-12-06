#pragma once

#include "DebugInterface.hpp"
#include "model/Model.hpp"
#include "drawer.h"
#include "world.h"
#include "game_status.h"
#include "astar.h"
#include "squads.h"
#include "actions.h"

World world;
GameStatus gameStatus;

class MyStrategy {
public:
    MyStrategy() {}
    ~MyStrategy() {}
    Action getAction(const PlayerView& playerView, DebugInterface* debugInterface) {
        cerr << "=================== Tick " << playerView.currentTick << "======================\n";
        maxDrawTick = playerView.currentTick;
        if (props.empty()) props = playerView.entityProperties;
        world.update(playerView);
        int resourcesLeft = playerView.players[playerView.myId - 1].resource;
        int myId = playerView.myId;
        std::unordered_map<int, EntityAction> moves;
        clearAStar();

        if (world.oppEntities.empty()) return Action();

        gameStatus.update(playerView, world);

        #ifdef DEBUG
        TickDrawInfo& info = tickInfo[playerView.currentTick];
        info.entities = playerView.entities;
        info.players = playerView.players;
        info.myId = playerView.myId;
        info.status = gameStatus;
        #endif

        calcSquadsTactic(world, gameStatus);

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

        unordered_map<int, int> healthLeft;
        for (const auto& e : playerView.entities) {
            healthLeft[e.id] = e.health;
        }

        for (const MyAction& action : actions) {
            auto [unitId, actionType, pos, oid, score] = action;
            const auto& upos = world.entityMap[unitId].position;
            const EntityType etype = static_cast<EntityType>(oid);

            if (usedUnits.find(unitId) != usedUnits.end()) continue;
            bool moved = false;
            const int cost = props.count(etype) ? props.at(etype).cost + world.myUnitsCnt[etype] : 0;
            
            switch (actionType) {
                case A_ATTACK:
                    if (healthLeft[oid] <= 0) {
                        continue;
                    }
                    healthLeft[oid] -= props.at(world.entityMap[unitId].entityType).attack->damage;
                    moves[unitId].attackAction = std::make_shared<AttackAction>(std::make_shared<int>(oid), nullptr);
                    info.msg[unitId] << "Attack " << oid << " at " << world.entityMap[oid].position;
                    break;
                case A_GATHER:
                    if (usedResources.insert(oid).second) {
                        if (dist(upos, pos) == 1) {
                            moves[unitId].attackAction = std::make_shared<AttackAction>(std::make_shared<int>(oid), nullptr);
                            info.msg[unitId] << "Gathers at " << world.entityMap[oid].position;
                        } else {
                            setMove(unitId, upos, pos);
                            moved = true;
                            info.msg[unitId] << "Moves to gather at " << pos;
                        }
                    } else {
                        continue;
                    }
                    break;
                case A_MOVE:
                    setMove(unitId, upos, pos);
                    moved = true;
                    info.msg[unitId] << "Move to " << pos;
                    break;
                case A_HIDE_MOVE:
                    hiding.insert(unitId);
                    setMove(unitId, upos, pos);
                    moved = true;
                    info.msg[unitId] << "Hide to " << pos;
                    break;
                case A_BUILD:
                case A_TRAIN:
                    if (cost <= resourcesLeft) {
                        moves[unitId].buildAction = std::make_shared<BuildAction>(etype, pos);
                        info.msg[unitId] << "Build/train [" << oid << "] at " << pos;
                        resourcesLeft -= cost;
                    } else {
                        continue;
                    }
                    break;
                case A_REPAIR:
                    moves[unitId].repairAction = std::make_shared<RepairAction>(oid);
                    info.msg[unitId] << "Repair " << oid;
                    // bestRepairScore = 200;
                    break;
                case A_REPAIR_MOVE:
                    if (bestRepairScore == -1 || score.main >= 199 || score.main <= -2000) {
                        if (score.main == -3000) gameStatus.workersLeftToFixTurrets = true;
                        setMove(unitId, upos, pos);
                        moved = true;
                        bestRepairScore = score.main;
                        info.msg[unitId] << "Repair_move to " << pos << " (score.main = " << score.main << ")" << endl;
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
        }

        sort(fmoves.begin(), fmoves.end());

        for (const auto [fi, se] : fmoves) {
            const int unitId = fi.second;
            const auto [from, to] = se;
            info.targets.emplace_back(from, to);
            
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
            
            // cerr << "set move target for " << from << "->" << to << " : " << target << endl;
            moves[unitId].moveAction = std::make_shared<MoveAction>(target, true, false);
            info.msg[unitId] << ", A* next: " << target;
        }

        // if (debugInterface) draw(playerView, *debugInterface);

        return Action(moves);
    }

    void debugUpdate(const PlayerView& playerView, DebugInterface& debugInterface) {
        // currentDrawTick = playerView.currentTick;
        // cerr << "currentDrawTick: " << currentDrawTick << endl;
    }
};