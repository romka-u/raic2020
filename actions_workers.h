#pragma once

#include "actions.h"

unordered_set<int> usedWorkers;
int uwit;
int uw[88][88];

int workerCellsReachableUWPrefilled(const World& world) {
    if (world.myWorkers.empty()) return 0;
    for (const auto& b : world.myBuildings)
        forn(x, b.size) forn(y, b.size)
            uw[b.position.x + x][b.position.y + y] = uwit;
    for (const auto& r : world.resources)
        uw[r.position.x][r.position.y] = uwit;

    vector<Cell> q;
    q.push_back(world.myWorkers.front().position);
    uw[q.back().x][q.back().y] = uwit;
    size_t qb = 0;

    int maxx = -1, maxy = -1;
    for (const auto& w : world.myWorkers) {
        if (w.position.x > maxx) maxx = w.position.x;
        if (w.position.y > maxy) maxy = w.position.y;
    }
    maxx += 4;
    maxy += 4;
    
    while (qb < q.size()) {
        const Cell cur = q[qb++];
        forn(w, 4) {
            const Cell nc = cur ^ w;
            if (nc.inside() && nc.x <= maxx && nc.y <= maxy && uw[nc.x][nc.y] != uwit) {
                uw[nc.x][nc.y] = uwit;
                q.push_back(nc);
            }
        }
    }
    return q.size();
}

bool workersAreOk(const World& world, const Cell& c, int sz, int wasReachable) {
    uwit++;
    forn(x, sz) forn(y, sz) {
        uw[c.x + x][c.y + y] = uwit;
    }
    const int curReachable = workerCellsReachableUWPrefilled(world);
    return curReachable >= wasReachable - sz * sz - 5;
}

bool canBuild(const World& world, const Cell& c, int sz, int wasReachable) {
    if (!c.inside()) return false;
    Cell cc = c + Cell(sz - 1, sz - 1);
    if (!cc.inside()) return false;
    forn(dx, sz) forn(dy, sz) {
        if (!world.isEmpty(c + Cell(dx, dy)))
            return false;
    }
    return workersAreOk(world, c, sz, wasReachable);
}

bool goodForTurret(const Cell& c, int sz) {
    if (c.x < 27 && c.y < 27) return false;
    // if (c.x % 3 != 0 || c.y % 3 != 0) return false;
    return true;
}

bool noTurretAheadAndBehind(const World& world, const Cell& c) {
    if (c.x < c.y) {
        int lx = max(0, c.x - 3);
        int rx = min(79, c.x + 3);
        for (int y = max(c.y - 7, 0); y < 70; y++)
            for (int x = lx; x <= rx; x++) {
                const int id = world.eMap[x][y];
                if (id < 0 && world.entityMap.at(-id).entityType == EntityType::TURRET && world.entityMap.at(-id).playerId == world.myId) {
                    return false;
                }
            }
    } else {
        int ly = max(0, c.y - 3);
        int ry = min(79, c.y + 3);
        for (int x = max(c.x - 7, 0); x < 70; x++)
            for (int y = ly; y <= ry; y++) {
                const int id = world.eMap[x][y];
                if (id < 0 && world.entityMap.at(-id).entityType == EntityType::TURRET && world.entityMap.at(-id).playerId == world.myId) {
                    return false;
                }
            }
    }
    return true;
}

bool safeToBuild(const World& world, const Cell& c, int sz, int arb) {
    for (const auto& oe : world.oppEntities) {
        if (oe.attackRange > 0)
            if (dist(oe.position, c, sz) <= oe.attackRange + arb)
                return false;
    }
    return true;
}

int cellsNear(const World& world, const Cell& c, int sz) {
    int res = 0;
    Cell cc;
    for (int d = -1; d <= sz; d++) {
        cc = c + Cell(d, -1);
        res += (cc.inside() && !world.isEmpty(cc) && world.eMap[cc.x][cc.y] < 0 && world.entityMap.at(-world.eMap[cc.x][cc.y]).playerId != -1);
        cc = c + Cell(-1, d);
        res += (cc.inside() && !world.isEmpty(cc) && world.eMap[cc.x][cc.y] < 0 && world.entityMap.at(-world.eMap[cc.x][cc.y]).playerId != -1);
        cc = c + Cell(d, sz);
        res += (cc.inside() && !world.isEmpty(cc) && world.eMap[cc.x][cc.y] < 0 && world.entityMap.at(-world.eMap[cc.x][cc.y]).playerId != -1);
        cc = c + Cell(sz, d);
        res += (cc.inside() && !world.isEmpty(cc) && world.eMap[cc.x][cc.y] < 0 && world.entityMap.at(-world.eMap[cc.x][cc.y]).playerId != -1);
    }
    return res;
}

int addBuildRanged(const World& world, vector<MyAction>& actions, const GameStatus& st, bool checkBorder) {
    if (st.needRanged != 1) return -1;

    int bestScore = -inf, bestId = -1;
    Cell bestPos;
    const int sz = props.at(EntityType::RANGED_BASE).size;
    unordered_map<Cell, bool> checkedPos;
    uwit++;
    const int wasReachable = workerCellsReachableUWPrefilled(world);

    for (const auto& wrk : world.myWorkers) {
        if (usedWorkers.find(wrk.id) != usedWorkers.end()) continue;
        
        for (Cell newPos : nearCells(wrk.position - Cell(sz - 1, sz - 1), sz)) {
            if (checkedPos.find(newPos) == checkedPos.end()) {
                checkedPos[newPos] = canBuild(world, newPos, sz, wasReachable);
            }
            if (checkedPos[newPos] && safeToBuild(world, newPos, sz, 15)) {
                int score = newPos.x + newPos.y - cellsNear(world, newPos, sz) * 4;
                if (score > bestScore) {
                    bestScore = score;
                    bestPos = newPos;
                    bestId = wrk.id;
                }
            }
        }
    }

    if (bestScore > -inf) {
        actions.emplace_back(bestId, A_BUILD, bestPos, EntityType::RANGED_BASE, Score(1001, bestScore));
        return bestId;
    }
    return -1;
}

int addBuildTurret(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    if (st.needRanged != 2) return -1;

    int bestScore = -inf, bestId = -1;
    Cell bestPos;
    const int sz = props.at(EntityType::RANGED_BASE).size;
    unordered_map<Cell, bool> checkedPos;
    uwit++;
    const int wasReachable = workerCellsReachableUWPrefilled(world);

    for (const auto& wrk : world.myWorkers) {
        if (usedWorkers.find(wrk.id) != usedWorkers.end()) continue;

        bool nearRes = false;
        forn(q, 4) {
            const Cell nc = wrk.position ^ q;
            if (nc.inside() && world.hasNonMovable(nc) && world.entityMap.at(-world.eMap[nc.x][nc.y]).entityType == EntityType::RESOURCE) {
                nearRes = true;
                break;
            }
        }
        /* if (!nearRes) */ continue;

        const int sz = props.at(EntityType::TURRET).size;
        for (Cell newPos : nearCells(wrk.position - Cell(sz - 1, sz - 1), sz)) {
            if (checkedPos.find(newPos) == checkedPos.end()) {
                checkedPos[newPos] = canBuild(world, newPos, sz, wasReachable);
            }
            if (checkedPos[newPos] && goodForTurret(newPos, sz) && noTurretAheadAndBehind(world, newPos) && safeToBuild(world, newPos, sz, 12)) {
                int score = -min(newPos.x, newPos.y);
                if (score > bestScore) {
                    bestId = wrk.id;
                    bestScore = score;
                    bestPos = newPos;
                }
            }
        }
    }

    if (bestScore > -inf) {
        actions.emplace_back(bestId, A_BUILD, bestPos, EntityType::TURRET, Score(700, bestScore));
        return bestId;
    }
    return -1;
}

int addBuildHouse(const World& world, vector<MyAction>& actions, const GameStatus& st, bool checkBorder) {
    int bestScore = -inf, bestId = -1;
    Cell bestPos;

    int housesInProgress = 0;
    for (const auto& b : world.myBuildings)
        if (b.entityType == EntityType::HOUSE && !b.active)
            housesInProgress++;

    if (world.finals) {
        if (housesInProgress > 1) return -1;
    } else {
        if (housesInProgress > 0) return -1;
    }
    const int sz = props.at(EntityType::HOUSE).size;
    unordered_map<Cell, bool> checkedPos;
    uwit++;
    const int wasReachable = workerCellsReachableUWPrefilled(world);

    const int MAX_FL = world.finals ? 200 : 145;
    for (const auto& wrk : world.myWorkers) {
        if (st.foodLimit >= st.foodUsed + 15 || st.foodLimit >= MAX_FL || st.needRanged == 1)
            break;
        if (usedWorkers.find(wrk.id) != usedWorkers.end()) continue;

        for (Cell newPos : nearCells(wrk.position - Cell(sz - 1, sz - 1), sz)) {
            if (checkedPos.find(newPos) == checkedPos.end()) {
                checkedPos[newPos] = canBuild(world, newPos, sz, wasReachable);
            }
            if (checkedPos[newPos] && safeToBuild(world, newPos, sz, 10)) {
                // if (newPos.x + newPos.y == 3 && !world.hasNonMovable({0, 0})) continue;
                int score = (newPos.x == 0) * 1000 + (newPos.y == 0) * 1000 - newPos.x - newPos.y;
                if (newPos.x > 0 && newPos.y > 0) score -= cellsNear(world, newPos, sz) * 5;
                if (score > bestScore) {
                    bestScore = score;
                    bestPos = newPos;
                    bestId = wrk.id;
                }
            }
        }
    }

    if (bestScore > -inf) {
        // cerr << bestId << " will build a house at " << bestPos << endl;
        actions.emplace_back(bestId, A_BUILD, bestPos, EntityType::HOUSE, Score(1000, bestScore));
        return bestId;
    }
    return -1;
}

void addRepairActions(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    bool anyToRepair = false;
    vector<pii> wrkList;
    for (const auto& wrk : world.myWorkers) {
        int pr = dRep[wrk.position.x][wrk.position.y];
        wrkList.emplace_back(pr, wrk.id);
    }

    sort(wrkList.begin(), wrkList.end());
    unordered_map<int, int> repairers;

    for (const auto& [pr, id] : wrkList) {
        if (pr > 1e5) break;
        if (usedWorkers.find(id) != usedWorkers.end()) continue;
        const auto& wrk = world.entityMap.at(id);

        pair<vector<Cell>, int> pathToRep = getPathToMany(world, wrk.position, dRep);
        const Entity& repTarget = world.entityMap.at(world.getIdAt(pathToRep.first.back()));
        int healthToRepair = repTarget.maxHealth - repTarget.health;
        int currentWorkers = repairers[repTarget.id];
        int htrwme = healthToRepair - currentWorkers * pathToRep.second;
        if (htrwme < 0) htrwme = 0;
        int ticksWithMe = htrwme / (currentWorkers + 1);

        if (pathToRep.second < 16 && ticksWithMe >= pathToRep.second) {
            int targetId = repTarget.id;
            repairers[repTarget.id]++;
            if (pathToRep.first.size() <= 2) {
                actions.emplace_back(id, A_REPAIR, NOWHERE, targetId, Score(120, 0));
            } else {
                actions.emplace_back(id, A_REPAIR_MOVE, pathToRep.first[1], targetId, Score(101, 0));
            }
            #ifdef DEBUG
            pathDebug[id].path = pathToRep.first;
            pathDebug[id].length = pathToRep.second;
            #endif
            updateAStar(world, pathToRep.first);
            usedWorkers.insert(id);
            continue;
        }
    }
}

void addHideActions(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    for (const auto& wrk : world.myWorkers) {
        if (usedWorkers.find(wrk.id) != usedWorkers.end()) continue;

        Cell threat(-1, -1);
        for (const auto& oe : world.oppEntities) {
            if (oe.entityType == EntityType::MELEE_UNIT || oe.entityType == EntityType::RANGED_UNIT)
                if (dist(wrk.position, oe.position) <= oe.attackRange + 2) {
                    threat = oe.position;
                    break;
                }
            if (oe.entityType == EntityType::TURRET)
                if (dist(wrk.position, oe) <= oe.attackRange) {
                    threat = oe.position;
                    break;
                }
        }

        if (threat.x != -1) {
            Cell target(-1, -1);
            int bestScore = -1234;
            forn(w, 5) {
                const Cell nc = wrk.position ^ w;
                if (nc.inside() && !world.hasNonMovable(nc)) {
                    int score = dist(nc, threat) * 2 - max(nc.x, nc.y);
                    if (score > bestScore) {
                        bestScore = score;
                        target = nc;
                    }
                }
            }

            if (target.x != -1) {
                vector<Cell> path = {wrk.position, target};
                actions.emplace_back(wrk.id, A_HIDE_MOVE, target, -1, Score(135, 0));
                updateAStar(world, path);
                usedWorkers.insert(wrk.id);
                #ifdef DEBUG
                pathDebug[wrk.id].path = path;
                pathDebug[wrk.id].length = 2;
                #endif
                continue;
            }
        }
    }
}

void addBuildActions(const World& world, vector<MyAction>& actions, const GameStatus& st, int& resources) {
    const int rbc = props.at(EntityType::RANGED_BASE).cost;
    if (resources >= rbc) {
        int wrkId = addBuildRanged(world, actions, st, resources < rbc * 1.64);
        if (wrkId != -1)
            resources -= rbc;
    }
    const int hbc = props.at(EntityType::HOUSE).cost;
    if (resources >= hbc) {
        int wrkId = addBuildHouse(world, actions, st, resources < hbc * 1.64 || st.needRanged == 2);
        if (wrkId != -1)
            resources -= hbc;
    }
    if (resources >= props.at(EntityType::TURRET).cost) {
        int wrkId = addBuildTurret(world, actions, st);
        if (wrkId != -1)
            resources -= props.at(EntityType::TURRET).cost;
    }
}

void addGatherActions(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    vector<pii> wrkList;
    for (const auto& wrk : world.myWorkers) {
        if (usedWorkers.find(wrk.id) != usedWorkers.end()) continue;
        int pr = dRes[wrk.position.x][wrk.position.y];
        wrkList.emplace_back(pr, wrk.id);
    }

    sort(wrkList.begin(), wrkList.end());
    for (const auto& [_, id] : wrkList) {
        const auto& wrk = world.entityMap.at(id);

        pair<vector<Cell>, int> pathToGo = getPathToMany(world, wrk.position, dRes);
        int targetId = world.getIdAt(pathToGo.first.back());

        if (pathToGo.second > 88) {
            continue;
        }

        if (pathToGo.first.size() <= 2) {
            actions.emplace_back(id, A_GATHER, NOWHERE, targetId, Score(120, 0));
        } else {
            actions.emplace_back(id, A_GATHER_MOVE, pathToGo.first[1], targetId, Score(101, 0));
        }
        updateAStar(world, pathToGo.first);
        usedWorkers.insert(id);
        #ifdef DEBUG
        pathDebug[id].path = pathToGo.first;
        pathDebug[id].length = pathToGo.second;
        #endif
    }
}

void addTurretsActions(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    vector<pii> wrkList;
    for (const auto& wrk : world.myWorkers) {
        if (usedWorkers.find(wrk.id) != usedWorkers.end()) continue;
        int pr = dTur[wrk.position.x][wrk.position.y];
        wrkList.emplace_back(pr, wrk.id);
    }

    sort(wrkList.begin(), wrkList.end());
    for (const auto& [pr, id] : wrkList) {
        if (pr > 1e5) break;
        const auto& wrk = world.entityMap.at(id);

        pair<vector<Cell>, int> pathToGo = getPathToMany(world, wrk.position, dTur);
        int targetId = world.getIdAt(pathToGo.first.back());

        if (pathToGo.first.size() <= 2) {
            actions.emplace_back(id, A_REPAIR, NOWHERE, targetId, Score(220, 0));
        } else {
            actions.emplace_back(id, A_REPAIR_MOVE, pathToGo.first[1], targetId, Score(201, 0));
        }
        updateAStar(world, pathToGo.first);
        usedWorkers.insert(id);
        #ifdef DEBUG
        pathDebug[id].path = pathToGo.first;
        pathDebug[id].length = pathToGo.second;
        #endif
    }
}

void addWorkersActions(const World& world, vector<MyAction>& actions, const GameStatus& st, int& resources) {
    usedWorkers.clear();
    addRepairActions(world, actions, st);
    addHideActions(world, actions, st);
    addBuildActions(world, actions, st, resources);
    addGatherActions(world, actions, st);
    addTurretsActions(world, actions, st);
}