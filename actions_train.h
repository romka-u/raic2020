#pragma once
#include "actions.h"

int ru[88][88], rit, rd[88][88];

void resBfs(const World& world) {
    rit++;
    vector<Cell> q;
    size_t qb = 0;
    for (const auto& r : world.resources) {
        if (r.health == r.maxHealth) {
            ru[r.position.x][r.position.y] = rit;
            q.push_back(r.position);
            rd[r.position.x][r.position.y] = 0;
        }
    }

    while (qb < q.size()) {
        Cell cur = q[qb++];
        forn(w, 4) {
            const Cell nc = cur ^ w;
            if (nc.inside() && ru[nc.x][nc.y] != rit) {
                ru[nc.x][nc.y] = rit;
                q.push_back(nc);
                rd[nc.x][nc.y] = rd[cur.x][cur.y] + 1;
            }
        }
    }
}

void getBestWorkerTrain(const World& world, const GameStatus& st, int& buildingId, Cell& trainPos) {
    buildingId = -1;
    int bestScore = -inf;
    for (const Entity& bu : world.myBuildings) {
        if (bu.entityType == EntityType::BUILDER_BASE) {
            for (Cell bornPlace : nearCells(bu.position, bu.size)) {
                if (bornPlace.inside() && world.isEmpty(bornPlace)) {
                    int score = -rd[bornPlace.x][bornPlace.y];
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

void getBestRangedTrain(const World& world, const GameStatus& st, const Cell& closestEnemy, int& buildingId, Cell& trainPos) {
    buildingId = -1;
    int bestScore = -inf;
    for (const Entity& bu : world.myBuildings) {
        if (bu.entityType == EntityType::RANGED_BASE) {
            for (Cell bornPlace : nearCells(bu.position, bu.size)) {
                if (bornPlace.inside() && world.isEmpty(bornPlace)) {
                    int score = -dist(bornPlace, closestEnemy);
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
    resBfs(world);

    int cld = inf;
    Cell closestEnemy(70, 70);
    for (const auto& oe : world.oppEntities) {
        for (const auto& e : world.myBuildings) {
            int cd = dist(oe.position, e);
            if (cd < cld) {
                cld = cd;
                closestEnemy = oe.position;
            }
        }
    }

    int workerBid, rangedBid;
    Cell workerTrainPos, rangedTrainPos;
    getBestWorkerTrain(world, st, workerBid, workerTrainPos);
    getBestRangedTrain(world, st, closestEnemy, rangedBid, rangedTrainPos);
    const int workerCost = props.at(EntityType::BUILDER_UNIT).cost + world.myUnitsCnt.at(EntityType::BUILDER_UNIT);
    const int rangedCost = props.at(EntityType::RANGED_UNIT).cost + world.myUnitsCnt.at(EntityType::RANGED_UNIT);

    const int myWS = world.workers[world.myId].size();
    const int WARRIORS_LIMIT = world.finals ? 111 : 77;
    const int WORKERS_LIMIT = world.finals ? 91 : 64;

    if (!needBuildArmy && myWS < 32 && resources < 100)
        rangedBid = -1;
    if (world.warriors[world.myId].size() >= WARRIORS_LIMIT)
        rangedBid = -1;

    if (st.workersLeftToFixTurrets
        || needBuildArmy
        || myWS >= min(WORKERS_LIMIT, int(st.resToGather.size() * 0.95))
        || !st.enemiesCloseToBase.empty())
        workerBid = -1;

    const int WS_FIRST_LIM = world.finals ? 64 : 42;
    bool workerFirst = (myWS <= WS_FIRST_LIM || world.tick % 8 == 0) && st.enemiesCloseToBase.empty();
    
    if (workerFirst) {
        if (resources >= workerCost && workerBid != -1) {
            resources -= workerCost;
            actions.emplace_back(workerBid, A_TRAIN, workerTrainPos, EntityType::BUILDER_UNIT, Score{420, 0});
        }
        if (resources >= rangedCost && rangedBid != -1) {
            resources -= rangedCost;
            actions.emplace_back(rangedBid, A_TRAIN, rangedTrainPos, EntityType::RANGED_UNIT, Score{410, 0});
        }
    } else {
        if (resources >= rangedCost && rangedBid != -1) {
            resources -= rangedCost;
            actions.emplace_back(rangedBid, A_TRAIN, rangedTrainPos, EntityType::RANGED_UNIT, Score{430, 0});
        }
        if (resources >= workerCost && workerBid != -1) {
            resources -= workerCost;
            actions.emplace_back(workerBid, A_TRAIN, workerTrainPos, EntityType::BUILDER_UNIT, Score{420, 0});
        }        
    }
}