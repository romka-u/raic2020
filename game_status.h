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

int isBorder[88][88], ubc[88][88], bgg[88][88], usp[88][88];
Cell clgc[88][88];

struct GameStatus {
    bool underAttack;
    vector<int> resToGather;
    int foodUsed, foodLimit;
    TurretStatus ts[KTS];
    int initialTurretId;
    bool workersLeftToFixTurrets;
    vector<Entity> buildingAttackers, enemiesCloseToBase;
    unordered_set<int> turretsInDanger;
    unordered_map<int, Cell> unitsToCell;
    unordered_map<int, vector<int>> utg[5];
    vector<int> attackersPower, attackersPowerClose;
    vector<Cell> hotPoints;
    int needRanged;
    int borderGroup[82][82], ubg[82][82], dtg[82][82], ubit;

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
        buildingAttackers.clear();
        enemiesCloseToBase.clear();
        for (int p = 1; p <= 4; p++) {
            if (p == world.myId) continue;
            for (int wi : world.warriors[p]) {
                const auto& w = world.entityMap.at(wi);
                bool pushedCB = false, pushedBA = false;
                for (const Entity& b : world.myBuildings) {
                    if (dist(w.position, b) <= w.attackRange && !pushedBA) {
                        underAttack = true;
                        buildingAttackers.push_back(w);
                        pushedBA = true;
                    }
                    if (dist(w.position, b) <= w.attackRange + 7 && !pushedCB) {
                        enemiesCloseToBase.push_back(w);
                        pushedCB = true;
                    }
                }
                if (!pushedCB) {
                    for (const Entity& wrk : world.myWorkers) {
                        if (dist(w.position, wrk.position) <= w.attackRange + 2 && !pushedCB) {
                            enemiesCloseToBase.push_back(w);
                            pushedCB = true;
                        }
                    }
                }
            }
        }
    }

    void updateResToGather(const World& world) {
        resToGather.clear();
        for (const Entity& res : world.resources) {
            if (world.infMap[res.position.x][res.position.y].first != world.myId) continue;
            bool coveredByEnemy = false;
            for (const auto& oe : world.oppEntities) {
                if (oe.entityType == EntityType::TURRET && dist(res.position, oe) < oe.attackRange) {
                    coveredByEnemy = true;
                    break;
                }
                if (oe.entityType == EntityType::RANGED_UNIT && dist(res.position, oe.position) < oe.attackRange + 2) {
                    coveredByEnemy = true;
                    break;
                }
            }
            if (!coveredByEnemy) {
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
                ts[0].target = Cell{10, 79 - 10};
                unordered_set<int> usedW;

                vector<Cell> path = getPathTo(world, HOME, {7, 75});
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
                    ts[0].target = Cell{10, 79 - 10};
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
                    ts[1].target = Cell{79 - 10, 10};
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

    bool calcBorderPoints(const World& world, vector<Cell> borderPoints[5]) {
        bool found = false;
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
                        found = true;
                        isBorder[x][y] = ubit;
                        break;
                    }
                }
            }
        return found;
    }

    void calcUnitsToCell(const World& world, vector<Cell> borderPoints[5]) {
        vector<Cell> q;
        size_t qb = 0;
        
        for (int p = 1; p <= 4; p++) {
            for (const Cell& c : borderPoints[p]) {
                q.emplace_back(c);
                clgc[c.x][c.y] = c;
                dtg[c.x][c.y] = 0;
                ubc[c.x][c.y] = ubit;
            }
        }

        while (qb < q.size()) {
            const Cell c = q[qb++];
            forn(w, 4) {
                const Cell nc = c ^ w;
                if (nc.inside() && ubc[nc.x][nc.y] != ubit) {
                    ubc[nc.x][nc.y] = ubit;
                    dtg[nc.x][nc.y] = dtg[c.x][c.y] + 1;
                    clgc[nc.x][nc.y] = clgc[c.x][c.y];
                    if (!world.hasNonMovable(nc))
                        q.push_back(nc);
                }
            }
        }

        for (int p = 1; p <= 4; p++) {
            for (int wi : world.warriors[p]) {
                const auto& w = world.entityMap.at(wi);
                const Cell& c = w.position;
                if (ubc[c.x][c.y] == ubit) {
                    unitsToCell[wi] = clgc[c.x][c.y];
                } else {
                    int cld = inf;
                    Cell bc(-1, -1);
                    for (const Cell& bp : borderPoints[p]) {
                        int cd = dist(c, bp);
                        if (cd < cld) {
                            cld = cd;
                            bc = bp;
                        }
                    }
                    if (bc.x != -1) {
                        unitsToCell[wi] = bc;
                    } else {
                        unitsToCell[wi] = c;
                    }
                }
            }
            for (int bi : world.buildings[p]) {
                const auto& b = world.entityMap.at(bi);
                const Cell& c = b.position;
                if (ubc[c.x][c.y] == ubit) {
                    unitsToCell[bi] = clgc[c.x][c.y];
                } else {
                    int cld = inf;
                    Cell bc(-1, -1);
                    for (const Cell& bp : borderPoints[p]) {
                        int cd = dist(c, bp);
                        if (cd < cld) {
                            cld = cd;
                            bc = bp;
                        }
                    }
                    if (bc.x != -1) {
                        unitsToCell[bi] = bc;
                    } else {
                        unitsToCell[bi] = c;
                    }
                }
            }
        }
    }

    void removeFar(vector<Cell> borderPoints[5]) {
        vector<pair<Cell, int>> q;
        size_t qb = 0;

        for (const auto& [unitId, c] : unitsToCell) {
            if (usp[c.x][c.y] != ubit) {
                usp[c.x][c.y] = ubit;
                q.emplace_back(c, 0);
            }
        }

        while (qb < q.size()) {
            auto [cur, d] = q[qb++];
            forn(w, 4) {
                const Cell nc = cur ^ w;
                if (nc.inside() && usp[nc.x][nc.y] != ubit) {
                    usp[nc.x][nc.y] = ubit;
                    if (d <= 11) {
                        q.emplace_back(nc, d + 1);
                    }
                }
            }
        }

        for (int p = 1; p <= 4; p++) {
            int nn = 0;
            forn(i, borderPoints[p].size()) {
                const Cell& c = borderPoints[p][i];
                if (usp[c.x][c.y] == ubit) {
                    borderPoints[p][nn++] = c;
                } else {
                    isBorder[c.x][c.y] = -1;
                }
            }
            borderPoints[p].resize(nn);
        }
    }

    int calcBorderGroups(const World& world, vector<Cell> borderPoints[5]) {
        forn(x, 79) forn(y, 79) {
            if (isBorder[x][y] == ubit && isBorder[x][y+1] == ubit && isBorder[x+1][y] == ubit && isBorder[x+1][y+1] == ubit) {
                int cntDiff = 0;
                for (int p = 1; p <= 4; p++) {
                    cntDiff += world.infMap[x][y].first == p || world.infMap[x][y+1].first == p || world.infMap[x+1][y].first == p || world.infMap[x+1][y+1].first == p;
                }
                if (cntDiff > 2) {
                    bgg[x][y] = bgg[x+1][y] = bgg[x][y+1] = bgg[x+1][y+1] = ubit;
                }
            }
        }

        int color = 0;
        for (int p = 1; p <= 4; p++)
            for (const Cell& cb : borderPoints[p]) {
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
                                borderGroup[nc.x][nc.y] = color;
                                if (bgg[nc.x][nc.y] != ubit)
                                    q.push_back(nc);
                            }
                        }
                    }
                    color++;
                }
            }
        return color;
    }

    void calcGroupsBalance(const World& world, int groupsCnt) {
        // cerr << "groupsCnt = " << groupsCnt << endl;
        attackersPower.assign(groupsCnt, 0);
        attackersPowerClose.assign(groupsCnt, 0);
        for (int p = 1; p <= 4; p++) utg[p].clear();

        for (const auto& [unitId, c] : unitsToCell) {
            const auto& u = world.entityMap.at(unitId);
            if (u.entityType != EntityType::MELEE_UNIT && u.entityType != EntityType::RANGED_UNIT && u.entityType != EntityType::TURRET)
                continue;
            if (dtg[u.position.x][u.position.y] <= 19) {
                int pwr = min(11, u.health);
                // if (u.entityType == EntityType::TURRET) pwr = 42;
                if (u.playerId != world.myId) {
                    attackersPower[borderGroup[c.x][c.y]] += pwr;
                    if (dtg[u.position.x][u.position.y] <= 11) {
                        attackersPowerClose[borderGroup[c.x][c.y]] += pwr;
                    }
                }
                utg[u.playerId][borderGroup[c.x][c.y]].push_back(unitId);
            }
        }
    }

    void calcHotPoints(const World& world, vector<Cell> borderPoints[5], int groupsCnt) {
        vector<vector<Cell>> groupCells(groupsCnt), unitCloseGroupCells(groupsCnt);
        // cerr << groupsCnt << endl;
        for (const auto& [unitId, c] : unitsToCell) {
            // cerr << "unit" << unitId << " maps to cell " << c;
            // cerr << ", borderGroup is " << borderGroup[c.x][c.y] << endl;
            const auto& u = world.entityMap.at(unitId);
            if (u.entityType != EntityType::MELEE_UNIT && u.entityType != EntityType::RANGED_UNIT && u.entityType != EntityType::TURRET)
                continue;
            int grId = borderGroup[c.x][c.y];
            unitCloseGroupCells[grId].push_back(c);
        }
        for (int p = 1; p <= 4; p++)
            for (const Cell& c : borderPoints[p]) {
                int grId = borderGroup[c.x][c.y];
                groupCells[grId].push_back(c);
            }

        hotPoints.resize(groupsCnt);
        forn(i, groupsCnt) {
            int score = inf;
            for (const Cell& c : groupCells[i]) {
                int cs = 0;
                for (const Cell& uc : unitCloseGroupCells[i])
                    cs += dist(uc, c);
                if (cs < score) {
                    score = cs;
                    hotPoints[i] = c;
                }
            }
        }
    }
    
    void updateHotPoints(const World& world) {
        vector<Cell> borderPoints[5];
        ubit++;
        unitsToCell.clear();

        if (!calcBorderPoints(world, borderPoints)) return;
        calcUnitsToCell(world, borderPoints);
        removeFar(borderPoints);
        int groupsCnt = calcBorderGroups(world, borderPoints);
        calcGroupsBalance(world, groupsCnt);
        calcHotPoints(world, borderPoints, groupsCnt);
    }

    void update(const World& world) {
        updateTurretsInDanger(world);
        updateUnderAttack(world);
        updateResToGather(world);
        updateFoodLimit(world);
        updateTurretsState(world);
        updateHotPoints(world);

        needRanged = 0;
        if (world.myWorkers.size() >= 16) {
            needRanged = 1;
            for (const auto& b : world.myBuildings)
                if (b.entityType == EntityType::RANGED_BASE && b.playerId == world.myId) {
                    needRanged = 2;
                    break;
                }
        }
    }
};
