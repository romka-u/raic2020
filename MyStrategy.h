#pragma once

#include "DebugInterface.hpp"
#include "model/Model.hpp"
#include "drawer.h"
#include "mytime.h"
#include "world.h"
#include "game_status.h"
#include "astar.h"
#include "actions_all.h"

World world;
GameStatus gameStatus;

class MyStrategy {
public:
    int prevGathered;
    int totalElapsed;
    MyStrategy() { totalElapsed = prevGathered = 0; }
    ~MyStrategy() {}

    bool weWin(const World& world, const PlayerView& playerView) const {
        if (world.oppEntities.size() > 4) return false;
        forn(p, playerView.players.size()) {
            if (p + 1 != world.myId && playerView.players[p].score >= playerView.players[world.myId - 1].score)
                return false;
        }
        for (const auto& e : world.oppEntities)
            if (e.entityType != EntityType::HOUSE && e.entityType != EntityType::BUILDER_UNIT)
                return false;
        return true;
    }

    Action getAction(const PlayerView& playerView, DebugInterface* debugInterface) {
        unsigned startTime = elapsed();
        cerr << "T" << playerView.currentTick << ":";
        #ifndef DEBUG
        if (totalElapsed > 373737) return Action();
        #endif
        maxDrawTick = playerView.currentTick;
        if (props.empty()) props = playerView.entityProperties;
        world.update(playerView);
        int resourcesLeft = playerView.players[playerView.myId - 1].resource + prevGathered;
        prevGathered = 0;
        int myId = playerView.myId;
        std::unordered_map<int, EntityAction> moves;
        clearAStar();
        unsigned tt = elapsed();
        gameStatus.update(world, playerView);
        // cerr << "gameStatus.update: " << (elapsed() - tt) << "ms.\n";
        updateD(world, gameStatus.resToGather);
        if (world.finals) {
            tt = elapsed();
            assignFinalsTargets(world, gameStatus);   
            cerr << "finals targets: " << (elapsed() - tt) << "ms.\n";
        } else {
            assignTargets(world, gameStatus);
        }
        tt = elapsed();
        assignFrontMoves(world, gameStatus);
        cerr << "front moves: " << (elapsed() - tt) << "ms.\n";

        TickDrawInfo& info = tickInfo[playerView.currentTick];
        #ifdef DEBUG
        pathDebug.clear();
        info.entities = world.allEntities;
        info.players = playerView.players;
        info.myId = playerView.myId;
        info.status = gameStatus;
        info.myPower = myPower;
        info.myFrontIds = myFrontIds;
        info.frontMoves = frontMoves;
        info.attackTargets.clear();
        for (const auto& [fi, ti] : attackTarget)
            info.attackTargets.emplace_back(world.entityMap.at(fi).position, world.entityMap.at(ti).position);
        #endif

        // return Action();

        vector<MyAction> actions;
        if (world.finals && weWin(world, playerView) && (gameStatus.ts.state == TS_FAILED || !TURRETS_CHEESE)) {
            if (world.myWarriors.size() + world.myWorkers.size() < horseRaw.size()) {
                addTrainActions(world, actions, gameStatus, resourcesLeft);
                addWorkersActions(world, actions, gameStatus, resourcesLeft);
                for (const auto& w : world.myWarriors)
                    actions.emplace_back(w.id, A_MOVE, Cell(63, 63), -1, Score(777));
            } else {
                addHorseActions(world, actions, gameStatus);
            }
        } else {
            tt = elapsed();
            addTrainActions(world, actions, gameStatus, resourcesLeft);
            cerr << "train actions: " << (elapsed() - tt) << "ms.\n";
            tt = elapsed();
            addWorkersActions(world, actions, gameStatus, resourcesLeft);
            cerr << "workers actions: " << (elapsed() - tt) << "ms.\n";
            tt = elapsed();
            addWarActions(world, actions, gameStatus);
            cerr << "war actions: " << (elapsed() - tt) << "ms.\n";
        }

        sort(actions.begin(), actions.end());

        unordered_set<int> usedUnits, usedResources;
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
    
        tt = elapsed();
        for (const MyAction& action : actions) {
            auto [unitId, actionType, pos, oid, score] = action;
            const auto& upos = world.entityMap[unitId].position;
            const EntityType etype = static_cast<EntityType>(oid);

            if (usedUnits.find(unitId) != usedUnits.end()) continue;
            bool moved = false;

            switch (actionType) {
                case A_ATTACK:
                    if (healthLeft[oid] <= 0) {
                        continue;
                    }
                    healthLeft[oid] -= props.at(world.entityMap[unitId].entityType).attack->damage;
                    moves[unitId].attackAction = std::make_shared<AttackAction>(std::make_shared<int>(oid), nullptr);
                    info.msg[unitId] << "Attack " << oid << " at " << world.entityMap[oid].position;
                    // cerr << unitId << " Attack " << oid << " at " << world.entityMap[oid].position << " ";
                    break;
                case A_GATHER:
                    moves[unitId].attackAction = std::make_shared<AttackAction>(std::make_shared<int>(oid), nullptr);
                    info.msg[unitId] << "Gathers " << world.entityMap[oid].position;
                    // cerr << unitId << " Gathers " << world.entityMap[oid].position << " ";
                    moved = true;
                    prevGathered++;
                    break;
                case A_GATHER_MOVE:
                    moves[unitId].moveAction = std::make_shared<MoveAction>(pos, true, false);
                    info.msg[unitId] << "Moves to " << pos << " to gather " << oid;
                    moved = true;
                    break;
                case A_MOVE:
                    setMove(unitId, upos, pos);
                    // moves[unitId].moveAction = std::make_shared<MoveAction>(pos, true, false);
                    moved = true;
                    info.msg[unitId] << "Moves to " << pos;
                    break;
                case A_HIDE_MOVE:
                    moves[unitId].moveAction = std::make_shared<MoveAction>(pos, true, false);
                    moved = true;
                    info.msg[unitId] << "Hide to " << pos;
                    // cerr << unitId << " Hide to " << pos << " ";
                    break;
                case A_BUILD:
                    moves[unitId].buildAction = std::make_shared<BuildAction>(etype, pos);
                    info.msg[unitId] << "Build [" << oid << "] at " << pos;
                    // cerr << unitId << " Build [" << oid << "] at " << pos << " ";
                    moved = true;
                    break;
                case A_TRAIN:
                    moves[unitId].buildAction = std::make_shared<BuildAction>(etype, pos);
                    info.msg[unitId] << "Train [" << oid << "] at " << pos;
                    moved = true;
                    break;
                case A_REPAIR:
                    moves[unitId].repairAction = std::make_shared<RepairAction>(oid);
                    info.msg[unitId] << "Repair " << oid;
                    moved = true;
                    #ifdef DEBUG
                    info.repairTargets.emplace_back(upos, world.entityMap[oid].position);
                    #endif
                    break;
                case A_REPAIR_MOVE:
                    moves[unitId].moveAction = std::make_shared<MoveAction>(pos, true, false);
                    info.msg[unitId] << "Move to " << pos << " to repair " << oid;
                    moved = true;
                    break;
                case A_FREE_MOVE:
                    setMove(unitId, upos, pos);
                    moved = true;
                    info.msg[unitId] << "Frees space, goes to " << pos;
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

        gameStatus.ts.failedToFindPath = false;
        for (const auto& [fi, se] : fmoves) {
            const int unitId = fi.second;
            const auto [from, to] = se;
            info.targets.emplace_back(from, to);

            vector<Cell> path = {from};
            if (frontTarget.count(unitId) && dist(from, to) == 1) {
                if (to != from) path = {from, to};
                moves[unitId].moveAction = std::make_shared<MoveAction>(to, true, false);
                info.msg[unitId] << ", by frontTarget";
                // cerr << "set path for " << unitId << " - no A*\n";
            } else {
                bool isRep = false;
                if (TURRETS_CHEESE) {
                    for (int wi : gameStatus.ts.repairers)
                        if (unitId == wi) {
                            isRep = true;
                            break;
                        }
                }
                // cerr << "path " << from << "->" << to << ", isRep = " << isRep << endl;
                path = getPathTo(world, from, to, isRep ? 19 : 4);
                // for (const auto& c : path) cerr << " " << c; cerr << endl;

                #ifdef DEBUG
                pathDebug[unitId].path = path;
                pathDebug[unitId].length = -1;
                #endif
                Cell target = path.size() >= 2 ? path[1] : to;

                if (world.hasNonMovable(target) && isRep) { 
                    gameStatus.ts.failedToFindPath = true;
                }

                moves[unitId].moveAction = std::make_shared<MoveAction>(target, true, false);
                info.msg[unitId] << ", A* next: " << target;
                // cerr << "set path for " << unitId << " with A*\n";
            }
            updateAStar(world, path);            
        }
        cerr << "perform actions & paths: " << (elapsed() - tt) << "ms.\n";

        #ifdef DEBUG
        info.pathDebug = pathDebug;
        #endif

        int timePerTick = elapsed() - startTime;
        totalElapsed += timePerTick;
        cerr << "> total: " << totalElapsed << " (" << timePerTick << " ms)\n";

        return Action(moves);
    }

    void debugUpdate(const PlayerView& playerView, DebugInterface& debugInterface) {
    }
};