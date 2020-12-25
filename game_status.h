#pragma once

#include "common.h"
#include "world.h"
#include "astar.h"


const int TS_NOT_BUILD = 0;
const int TS_PLANNED = 1;
const int TS_BUILDING = 2;
const int TS_FAILED = 3;

struct TurretStatus {
    int state;
    bool failedToFindPath;
    vector<int> repairers;

    TurretStatus() {
        state = TS_NOT_BUILD;
        failedToFindPath = false;
    }
};

int isBorder[88][88], ubc[88][88], bgg[88][88], usp[88][88];
Cell clgc[88][88];

struct GameStatus {
    bool underAttack;
    vector<int> resToGather;
    int foodUsed, foodLimit;
    TurretStatus ts;
    int initialTurretId;
    bool workersLeftToFixTurrets;
    vector<Entity> buildingAttackers;
    unordered_set<int> closeToBaseIds;
    vector<Entity> enemiesCloseToWorkers;
    unordered_set<int> turretsInDanger;
    unordered_map<int, Cell> unitsToCell;
    unordered_map<int, vector<int>> utg[5];
    vector<int> attackersPower, attackersPowerClose;
    vector<Cell> hotPoints;
    int needRanged;
    int borderGroup[82][82], ubg[82][82], dtg[82][82], ubit;
    int prevOppResources;

    GameStatus() {
        prevOppResources = 0;
        needRanged = 0;
    }

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
        closeToBaseIds.clear();
        enemiesCloseToWorkers.clear();
        for (int p = 1; p <= 4; p++) {
            if (p == world.myId) continue;
            for (int bi : world.buildings[p]) {
                const auto& b = world.entityMap.at(bi);
                if (b.entityType == EntityType::TURRET && b.active)
                    for (const Entity& wrk : world.myWorkers)
                        if (dist(wrk.position, b) <= b.attackRange) {
                            enemiesCloseToWorkers.push_back(b);
                            break;
                        }
            }
            for (int wi : world.warriors[p]) {
                const auto& w = world.entityMap.at(wi);
                bool pushedCB = false, pushedBA = false;
                if (w.entityType == EntityType::RANGED_UNIT || w.entityType == MELEE_UNIT)
                    for (const Entity& wrk : world.myWorkers)
                        if (dist(wrk.position, w.position) <= w.attackRange + 2) {
                            enemiesCloseToWorkers.push_back(w);
                            break;
                        }
                for (const Entity& b : world.myBuildings) {
                    if (dist(w.position, b) <= w.attackRange && !pushedBA) {
                        underAttack = true;
                        buildingAttackers.push_back(w);
                        pushedBA = true;
                    }
                    if (dist(w.position, b) <= w.attackRange + 7 && !pushedCB) {
                        closeToBaseIds.insert(w.id);
                        pushedCB = true;
                    }
                }
                if (!pushedCB) {
                    for (const Entity& wrk : world.myWorkers) {
                        if (dist(w.position, wrk.position) <= w.attackRange + 2 && !pushedCB) {
                            closeToBaseIds.insert(w.id);
                            pushedCB = true;
                        }
                    }
                }
            }
        }
        sort(enemiesCloseToWorkers.begin(), enemiesCloseToWorkers.end(),
             [](const Entity& a, const Entity& b)
             { return a.position.x * a.position.x + a.position.y * a.position.y < b.position.x * b.position.x + b.position.y * b.position.y; });
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
        if (ts.failedToFindPath) {
            bool haveAnyTurret = false;
            for (const auto& b : world.myBuildings)
                if (b.entityType == EntityType::TURRET) {
                    haveAnyTurret = true;
                    break;
                }
            if (!haveAnyTurret) {
                ts.state = TS_FAILED;
                ts.repairers.clear();
                return;
            }
        }

        if (needRanged > 0) {
            ts.state = TS_FAILED;
            ts.repairers.clear();
            return;
        }

        if (foodLimit >= 10 && TURRETS_CHEESE && world.finals) {
            if (ts.state == TS_NOT_BUILD) {
                int dBase[82][82];
                memset(dBase, 0, sizeof(dBase));
                uit++;
                vector<Cell> q;
                q.emplace_back(79, 79);
                goBfs(world, q, dBase);
                unordered_set<int> usedW;
                forn(it, 4) {
                    int cld = inf, ci = -1;
                    for (const auto& w : world.myWorkers)
                        if (usedW.find(w.id) == usedW.end()) {
                            int cd = dBase[w.position.x][w.position.y];
                            if (cd == 0) {
                                ts.state = TS_FAILED;
                                break;
                            }
                            if (cd < cld) {
                                cld = cd;
                                ci = w.id;
                            }
                        }
                    if (ts.state == TS_FAILED) break;
                    usedW.insert(ci);
                    ts.repairers.push_back(ci);
                }
                if (ts.state != TS_FAILED) {
                    ts.state = TS_PLANNED;
                }
            }
        }

        int nn = 0;
        forn(i, ts.repairers.size()) {
            if (world.entityMap.count(ts.repairers[i])) {
                ts.repairers[nn++] = ts.repairers[i];
            }
        }
        ts.repairers.resize(nn);

        if (ts.state == TS_PLANNED) {
            for (const auto& b : world.myBuildings)
                if (b.entityType == EntityType::TURRET && !b.active) {
                    ts.state = TS_BUILDING;
                }
        }

        if (ts.state == TS_BUILDING) {
            ts.state = TS_PLANNED;
            for (const auto& b : world.myBuildings)
                if (b.entityType == EntityType::TURRET && !b.active) {
                    ts.state = TS_BUILDING;
                }
        }

        for (const auto& oe : world.oppEntities)
            if (oe.entityType == EntityType::RANGED_UNIT || oe.entityType == EntityType::MELEE_UNIT)
                ts.state = TS_FAILED;

        if (ts.state == TS_FAILED) {
            ts.repairers.clear();
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
                const int pwr = min(11, u.health);
                const int grId = borderGroup[c.x][c.y];
                // cerr << "unit " << u.id << " at " << u.position << " adds " << pwr << " power to group " << (char)('A' + grId) << endl;
                // if (u.entityType == EntityType::TURRET) pwr = 42;
                if (u.playerId != world.myId) {
                    attackersPower[grId] += pwr;
                    if (dtg[u.position.x][u.position.y] <= 11) {
                        attackersPowerClose[grId] += pwr;
                    }
                }
                utg[u.playerId][grId].push_back(unitId);
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
            int grId = min(borderGroup[c.x][c.y], groupsCnt - 1);
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

    void update(const World& world, const PlayerView& playerView) {
        updateTurretsInDanger(world);
        updateUnderAttack(world);
        updateResToGather(world);
        updateFoodLimit(world);
        updateTurretsState(world);
        if (!world.finals)
            updateHotPoints(world);

        if (needRanged == 2) needRanged = 0;
        for (const auto& b : world.myBuildings)
            if (b.entityType == EntityType::RANGED_BASE && b.playerId == world.myId) {
                needRanged = 2;
                break;
            }

        if (needRanged < 2) {
            if (world.finals) {
                const int oppId = 3 - world.myId;
                const int oppResources = playerView.players[oppId - 1].resource;
                if (oppResources < prevOppResources - 300 || world.myWorkers.size() >= 55 || world.tick > 357) {
                    needRanged = 1;
                }
                prevOppResources = oppResources;
            } else {
                if (world.myWorkers.size() >= 16) {
                    needRanged = 1;
                }
            }
        }
    }
};
