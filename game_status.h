#pragma once

#include "common.h"
#include "world.h"
#include "astar.h"

const int KTS = 2;

const int TS_NOT_BUILD = 0;
const int TS_PLANNED = 1;
const int TS_BUILT = 2;
const int TS_FAILED = 3;

struct TurretStatus {
    int state;
    Cell target;
    int turretId;
    vector<int> repairers;

    TurretStatus() {
        state = TS_NOT_BUILD;
    }
};

struct GameStatus {
    bool underAttack;
    vector<int> resToGather;
    int foodUsed, foodLimit;
    TurretStatus ts[KTS];
    int initialTurretId;
    bool workersLeftToFixTurrets;
    vector<Entity> attackers;

    void update(const PlayerView& playerView, const World& world) {
        int myId = playerView.myId;

        underAttack = false;
        attackers.clear();
        for (int p = 1; p <= 4; p++) {
            if (p == myId) continue;
            for (int wi : world.warriors[p]) {
                const auto& w = world.entityMap.at(wi);
                for (const Entity& b : world.myBuildings) {
                    if (dist(w.position, b) <= w.attackRange + 10) {
                        underAttack = true;
                        attackers.push_back(w);
                    }
                }
            }
        }

        resToGather.clear();
        for (const Entity& res : world.resources) {
            if (world.infMap[res.position.x][res.position.y] != myId) continue;
            bool coveredByTurret = false;
            for (const auto& oe : world.oppEntities) {
                if (oe.entityType == EntityType::TURRET && dist(res.position, oe) < oe.attackRange) {
                    coveredByTurret = true;
                    break;
                }
            }
            if (!coveredByTurret) {
                resToGather.push_back(res.id);
            }
        }

        foodUsed = 0;
        foodLimit = 0;
        for (int bi : world.buildings[myId])
            if (world.entityMap.at(bi).active)
                foodLimit += world.P(bi).populationProvide;
        for (int bi : world.workers[myId])
            foodUsed += world.P(bi).populationUse;
        for (int bi : world.warriors[myId])
            foodUsed += world.P(bi).populationUse;

        if (foodLimit >= 30 && TURRETS_CHEESE) {
            if (ts[0].state == TS_NOT_BUILD) {
                ts[0].target = Cell{7, 80 - 7};
                unordered_set<int> usedW;

                vector<Cell> path = getPathTo(world, Cell{10, 10}, {7, 75});
                bool ok = true;
                cerr << "path.size(): " << path.size() << endl;
                for (const Cell& c : path) {
                    if (30 <= c.x && c.x < 50 && 30 <= c.y && c.y < 50) {
                        ok = false;
                        break;
                    }
                    cerr << " " << c;
                }
                cerr << endl;
                if (path.size() < 10 || !ok) {
                    ts[0].state = TS_FAILED;
                    ts[1].state = TS_FAILED;                    
                } else {
                    forn(j, 2) {
                        int bw = -1;
                        int cld = inf;
                        for (const Entity& w : world.myWorkers) {
                            if (usedW.find(w.id) == usedW.end()) {
                                const int cd = dist(ts[0].target, w.position);
                                if (cd < cld) {
                                    cld = cd;
                                    bw = w.id;
                                }
                            }
                        }
                        ts[0].repairers.push_back(bw);
                        usedW.insert(bw);
                    }
                    ts[0].state = TS_PLANNED;
                    
                    ts[1].target = Cell{80 - 7, 7};
                    forn(j, 2) {
                        int bw = -1;
                        int cld = inf;
                        for (const Entity& w : world.myWorkers) {
                            if (usedW.find(w.id) == usedW.end()) {
                                const int cd = dist(ts[1].target, w.position);
                                if (cd < cld) {
                                    cld = cd;
                                    bw = w.id;
                                }
                            }
                        }
                        ts[1].repairers.push_back(bw);
                        usedW.insert(bw);
                    }
                    ts[1].state = TS_PLANNED;
                    cerr << "assigned cheese:";
                    for (int w : ts[0].repairers) cerr << " " << w;
                    cerr << " and";
                    for (int w : ts[1].repairers) cerr << " " << w;
                    cerr << endl;

                    for (const auto& t : world.myBuildings)
                        if (t.entityType == EntityType::TURRET)
                            initialTurretId = t.id;
                }
            }
        }

        forn(i, KTS) {
            if (!TURRETS_CHEESE) break;
            if (ts[i].state == TS_PLANNED) {
                for (int wi : ts[i].repairers) {
                    if (world.entityMap.count(wi)) {
                        const auto& w = world.entityMap.at(wi);

                        bool hasTurretNear = false;
                        for (const auto& t : world.myBuildings)
                            if (t.entityType == EntityType::TURRET && dist(w.position, t.position) <= 4 && t.id != initialTurretId) {
                                ts[i].state = TS_BUILT;
                                ts[i].turretId = t.id;
                                cerr << "turret " << i << " is built\n";
                                break;
                            }
                    } else {
                        ts[i].state = TS_FAILED;
                    }
                }
            } else {
                int nn = 0;
                for (int j = 0; j < ts[i].repairers.size(); j++)
                    if (world.entityMap.count(ts[i].repairers[j]))
                        ts[i].repairers[nn++] = ts[i].repairers[j];
                ts[i].repairers.resize(nn);
            }

            if (ts[i].state == TS_BUILT) {
                if (world.entityMap.count(ts[i].turretId)) {
                    while (ts[i].repairers.size() < 2) {
                        const int w1 = ts[i].repairers.empty() ? -1 : ts[i].repairers.back();
                        const Cell& pos1 = world.entityMap.at(w1).position;
                        int cld = inf;
                        int cw = -1;                    
                        for (int wi : world.workers[world.myId])
                            if (wi != w1) {
                                const int cd = dist(world.entityMap.at(wi).position, pos1);
                                if (cd < cld) {
                                    cld = cd;
                                    cw = wi;
                                }
                            }
                        if (cw == -1) break;
                        ts[i].repairers.push_back(cw);
                        cerr << "add " << cw << " to list of turret " << i << " repairers\n";
                    }
                } else {
                    ts[i].state = TS_FAILED;
                }
            }
        }
    }
};