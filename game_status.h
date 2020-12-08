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

int isBorder[88][88];

struct GameStatus {
    bool underAttack;
    vector<int> resToGather;
    int foodUsed, foodLimit;
    TurretStatus ts[KTS];
    int initialTurretId;
    bool workersLeftToFixTurrets;
    vector<Entity> attackers;
    unordered_set<int> turretsInDanger;
    unordered_map<int, Cell> unitsToCell;
    unordered_map<int, vector<int>> utg[5];
    vector<int> groupsBalance;
    int borderGroup[82][82], ubg[82][82], ubit;

    void updateTurretsInDanger(const World& world) {
        turretsInDanger.clear();
        for (const auto& t : world.myBuildings)
            if (t.entityType == EntityType::TURRET) {
                for (const auto& oe : world.oppEntities)
                    if (oe.entityType == EntityType::RANGED_UNIT || oe.entityType == EntityType::MELEE_UNIT)
                        if (dist(oe.position, t) < 8) {
                            turretsInDanger.insert(t.id);
                            break;
                        }
            }
    }

    void updateUnderAttack(const World& world) {
        underAttack = false;
        attackers.clear();
        for (int p = 1; p <= 4; p++) {
            if (p == world.myId) continue;
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
    }

    void updateResToGather(const World& world) {
        resToGather.clear();
        for (const Entity& res : world.resources) {
            if (world.infMap[res.position.x][res.position.y].first != world.myId) continue;
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
    }

    void updateFoodLimit(const World& world) {
        foodUsed = 0;
        foodLimit = 0;
        for (const auto& b : world.myBuildings)
            if (b.active)
                foodLimit += props.at(b.entityType).populationProvide;
        for (const auto& w : world.myWorkers)
            foodUsed += props.at(w.entityType).populationUse;
        for (const auto& w : world.myWarriors)
            foodUsed += props.at(w.entityType).populationUse;
    }
    
    void updateTurretsState(const World& world) {
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

    void calcBorderPoints(const World& world, vector<Cell> borderPoints[5]) {
        ubit++;
        forn(x, 80) forn(y, 80)
            if (world.infMap[x][y].first > 0 && !world.hasNonMovable({x, y})) {
                const int curId = world.infMap[x][y].first;
                forn(q, 4) {
                    Cell nc{x + dx[q], y + dy[q]};
                    if (nc.inside()
                        && !world.hasNonMovable(nc)
                        && world.infMap[nc.x][nc.y].first != 0
                        && world.infMap[nc.x][nc.y].first != curId) {
                        borderPoints[world.infMap[x][y].first].emplace_back(x, y);
                        isBorder[x][y] = ubit;
                        break;
                        // borderInd[x][y] = borderPoints.size() - 1;
                    }
                }
            }
    }

    void calcUnitsToCell(const World& world, vector<Cell> borderPoints[5]) {
        unitsToCell.clear();
        for (int p = 1; p <= 4; p++) {
            for (int wi : world.warriors[p]) {
                const auto& w = world.entityMap.at(wi);
                int cld = inf;
                Cell cc{7, 7};
                for (const Cell& c : borderPoints[p]) {
                    int cd = dist(c, w.position);
                    if (cd < cld) {
                        cld = cd;
                        cc = c;
                    }
                }
                unitsToCell[wi] = cc;
            }
            for (int bi : world.buildings[p]) {
                const auto& b = world.entityMap.at(bi);
                if (b.entityType != EntityType::TURRET) continue;
                int cld = inf;
                Cell cc{7, 7};
                for (const Cell& c : borderPoints[p]) {
                    int cd = dist(c, b.position);
                    if (cd < cld) {
                        cld = cd;
                        cc = c;
                    }
                }
                unitsToCell[bi] = cc;
            }
        }
    }

    int calcBorderGroups(vector<Cell> borderPoints[5]) {
        int color = 0;
        for (int p = 1; p <= 4; p++)
            forn(i, borderPoints[p].size()) {
                const Cell& cb = borderPoints[p][i];
                if (ubg[cb.x][cb.y] != ubit) {
                    ubg[cb.x][cb.y] = ubit;
                    borderGroup[cb.x][cb.y] = color;
                    vector<Cell> q;
                    q.push_back(cb);                
                    size_t qb = 0;

                    while (qb < q.size()) {
                        Cell cur = q[qb++];
                        forn(w, 4) {
                            const Cell nc = cur ^ w;
                            if (nc.inside() && isBorder[nc.x][nc.y] == ubit && ubg[nc.x][nc.y] != ubit) {
                                ubg[nc.x][nc.y] = ubit;
                                q.push_back(nc);
                                borderGroup[nc.x][nc.y] = color;
                            }
                        }
                    }
                    color++;
                }
            }
        return color;
    }

    void calcGroupsBalance(const World& world, const unordered_map<int, Cell>& unitsToCell, int groupsCnt) {
        groupsBalance.assign(groupsCnt, 0);
        for (int p = 1; p <= 4; p++) utg[p].clear();

        for (const auto& [unitId, c] : unitsToCell) {
            const auto& u = world.entityMap.at(unitId);
            if (dist(c, u) <= 11) {
                int pwr = 10;
                // if (u.entityType == EntityType::TURRET) pwr = 42;
                if (u.playerId == world.myId) {
                    groupsBalance[borderGroup[c.x][c.y]] += pwr;
                } else {
                    groupsBalance[borderGroup[c.x][c.y]] -= pwr;
                }
                utg[u.playerId][borderGroup[c.x][c.y]].push_back(unitId);
            }
        }
    }
    
    void updateHotPoints(const World& world) {
        vector<Cell> borderPoints[5];
        calcBorderPoints(world, borderPoints);
        for (int p = 1; p <= 4; p++) {
            cerr << p << ":";
            for (const auto& cell : borderPoints[p]) cerr << " " << cell;
            cerr << endl;                
        }
        int groupsCnt = calcBorderGroups(borderPoints);        
        calcUnitsToCell(world, borderPoints);
        calcGroupsBalance(world, unitsToCell, groupsCnt);
    }

    void update(const World& world) {
        updateTurretsInDanger(world);
        updateUnderAttack(world);
        updateResToGather(world);
        updateFoodLimit(world);
        updateTurretsState(world);
        updateHotPoints(world);
    }
};