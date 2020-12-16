#pragma once
#include "actions.h"

void getBestWorkerTrain(const World& world, const GameStatus& st, int& buildingId, Cell& trainPos) {
    const int myWS = world.workers[world.myId].size();
    buildingId = -1;
    int bestScore = -inf;
    for (const Entity& bu : world.myBuildings) {
        if (bu.entityType == EntityType::BUILDER_BASE
            && !st.workersLeftToFixTurrets
            && !needBuildArmy
            && myWS < min(64, int(st.resToGather.size() * 0.91))) {
            for (Cell bornPlace : nearCells(bu.position, bu.size)) {
                if (world.isEmpty(bornPlace)) {
                    int score = -dRes[bornPlace.x][bornPlace.y];
                    if (score > bestScore) {
                        bestScore = score;
                        buildingId = bu.id;
                        trainPos = bornPlace;
                    }
                }
            }
        }
    }
}

void getBestRangedTrain(const World& world, const GameStatus& st, int& buildingId, Cell& trainPos) {
    const int myWS = world.workers[world.myId].size();
    buildingId = -1;
    int bestScore = -inf;
    for (const Entity& bu : world.myBuildings) {
        if (bu.entityType == EntityType::RANGED_BASE
            && (needBuildArmy || myWS > 32)
            && world.warriors[world.myId].size() < 77) {
            for (Cell bornPlace : nearCells(bu.position, bu.size)) {
                if (world.isEmpty(bornPlace)) {
                    int score = bornPlace.x + bornPlace.y;
                    if (score > bestScore) {
                        bestScore = score;
                        buildingId = bu.id;
                        trainPos = bornPlace;
                    }
                }
            }
        }
    }
}

void addTrainActions(const World& world, vector<MyAction>& actions, const GameStatus& st, int& resources) {
    /*int gap = st.underAttack ? 0 : 1;
    for (const Entity& bu : world.myBuildings)
        if (!bu.active && bu.entityType == EntityType::HOUSE)
            gap = 0;*/
    int gap = 0;

    if (st.foodUsed >= st.foodLimit - gap) return;

    int workerBid, rangedBid;
    Cell workerTrainPos, rangedTrainPos;
    getBestWorkerTrain(world, st, workerBid, workerTrainPos);
    getBestRangedTrain(world, st, rangedBid, rangedTrainPos);
    const int workerCost = props.at(EntityType::BUILDER_UNIT).cost + world.myUnitsCnt.at(EntityType::BUILDER_UNIT);
    const int rangedCost = props.at(EntityType::RANGED_UNIT).cost + world.myUnitsCnt.at(EntityType::RANGED_UNIT);

    const int myWS = world.workers[world.myId].size();
    if (myWS > 42) {
        if (resources >= workerCost && workerBid != -1) {
            resources -= workerCost;
            actions.emplace_back(workerBid, A_TRAIN, workerTrainPos, EntityType::BUILDER_UNIT, Score{420, 0});
        }
        if (resources >= rangedCost && rangedBid != -1) {
            resources -= rangedCost;
            actions.emplace_back(rangedBid, A_TRAIN, rangedTrainPos, EntityType::RANGED_UNIT, Score{myWS * 10, 0});
        }
    } else {
        if (resources >= rangedCost && rangedBid != -1) {
            resources -= rangedCost;
            actions.emplace_back(rangedBid, A_TRAIN, rangedTrainPos, EntityType::RANGED_UNIT, Score{myWS * 10, 0});
        }
        if (resources >= workerCost && workerBid != -1) {
            resources -= workerCost;
            actions.emplace_back(workerBid, A_TRAIN, workerTrainPos, EntityType::BUILDER_UNIT, Score{420, 0});
        }        
    }
}