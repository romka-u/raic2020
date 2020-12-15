#pragma once

#include "actions.h"


bool canBuild(const World& world, const Cell& c, int sz) {
    if (!c.inside()) return false;
    Cell cc = c + Cell(sz - 1, sz - 1);
    if (!cc.inside()) return false;
    forn(dx, sz) forn(dy, sz) {
        if (!world.isEmpty(c + Cell(dx, dy)))
            return false;
    }
    if (c.x != 0 && c.y != 0) {
        for (int d = -1; d <= sz; d++) {
            cc = c + Cell(d, -1);
            if (cc.inside() && !world.isEmpty(cc) && world.eMap[cc.x][cc.y] < 0 && world.entityMap.at(-world.eMap[cc.x][cc.y]).playerId != -1) return false;
            cc = c + Cell(-1, d);
            if (cc.inside() && !world.isEmpty(cc) && world.eMap[cc.x][cc.y] < 0 && world.entityMap.at(-world.eMap[cc.x][cc.y]).playerId != -1) return false;
            cc = c + Cell(d, sz);
            if (cc.inside() && !world.isEmpty(cc) && world.eMap[cc.x][cc.y] < 0 && world.entityMap.at(-world.eMap[cc.x][cc.y]).playerId != -1) return false;
            cc = c + Cell(sz, d);
            if (cc.inside() && !world.isEmpty(cc) && world.eMap[cc.x][cc.y] < 0 && world.entityMap.at(-world.eMap[cc.x][cc.y]).playerId != -1) return false;
        }
    }
    return true;
}
/*
bool goodForHouse(const Cell& c, int sz) {
    if (c.x == 0) return c.y % sz == 1;
    if (c.y == 0) return c.x % sz == 1;
    if (c.x < sz || c.y < sz) return false;
    return (c.x % (sz + 2) == 1 || c.x == 3) && (c.y % (sz + 2) == 1 || c.y == 3);
}
*/
bool goodForTurret(const Cell& c, int sz) {
    if (c.x < 27 && c.y < 27) return false;
    // if (c.x % 3 != 0 || c.y % 3 != 0) return false;
    return true;
}

bool noTurretAhead(const World& world, const Cell& c) {
    if (c.x < c.y) {
        int lx = max(0, c.x - 3);
        int rx = min(79, c.x + 3);
        for (int y = c.y + 1; y < 70; y++)
            for (int x = lx; x <= rx; x++) {
                const int id = world.eMap[x][y];
                if (id < 0 && world.entityMap.at(-id).entityType == EntityType::TURRET && world.entityMap.at(-id).playerId == world.myId) {
                    return false;
                }
            }
    } else {
        int ly = max(0, c.y - 3);
        int ry = min(79, c.y + 3);
        for (int x = c.x + 1; x < 70; x++)
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

int addBuildRanged(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    int bestScore = -inf, bestId = -1;
    Cell bestPos;
    for (const auto& wrk : world.myWorkers) {
        if (st.needRanged != 1)
            break;
            
        const int sz = props.at(EntityType::RANGED_BASE).size;
        for (Cell newPos : nearCells(wrk.position - Cell(sz - 1, sz - 1), sz)) {
            if (canBuild(world, newPos, sz) && safeToBuild(world, newPos, sz, 15)) {
                int score = newPos.x + newPos.y;
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

int addBuildHouse(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    int bestScore = -inf, bestId = -1;
    Cell bestPos;

    int housesInProgress = 0;
    for (const auto& b : world.myBuildings)
        if (b.entityType == EntityType::HOUSE && !b.active)
            housesInProgress++;

    if (housesInProgress > 0) return -1;

    for (const auto& wrk : world.myWorkers) {
        if (st.foodLimit >= st.foodUsed + 15 || st.foodLimit >= 145 || st.needRanged == 1)
            break;
            
        const int sz = props.at(EntityType::HOUSE).size;
        for (Cell newPos : nearCells(wrk.position - Cell(sz - 1, sz - 1), sz)) {
            if (canBuild(world, newPos, sz) && safeToBuild(world, newPos, sz, 10)) {
                // if (newPos.x + newPos.y == 3 && !world.hasNonMovable({0, 0})) continue;
                int score = (newPos.x == 0) * 1000 + (newPos.y == 0) * 1000 - newPos.x - newPos.y;
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

void addWorkersActions(const World& world, vector<MyAction>& actions, const GameStatus& st, int& resources) {
    int wrkIdToBuildRanged = -1;
    int wrkIdToBuildHouse = -1;
    if (resources >= props.at(EntityType::RANGED_BASE).cost) {
        wrkIdToBuildRanged = addBuildRanged(world, actions, st);
        if (wrkIdToBuildRanged != -1)
            resources -= props.at(EntityType::RANGED_BASE).cost;
    }
    if (resources >= props.at(EntityType::HOUSE).cost) {
        wrkIdToBuildHouse = addBuildHouse(world, actions, st);
        if (wrkIdToBuildHouse != -1)
            resources -= props.at(EntityType::HOUSE).cost;
    }

    vector<pii> wrkList;
    bool anyToRepair = false;
    unordered_map<int, Cell> wrkThreat;
    for (const auto& wrk : world.myWorkers) {
        if (wrk.id == wrkIdToBuildHouse || wrk.id == wrkIdToBuildRanged) continue;
        int pr = inf;
        for (const auto& r : world.resources)
            pr = min(pr, dist(wrk.position, r.position));
        for (const auto& b : world.myBuildings)
            if (b.health < b.maxHealth) {
                pr = min(pr, dist(wrk.position, b));
                anyToRepair = true;
            }

        for (const auto& oe : world.oppEntities) {
            if (oe.entityType == EntityType::MELEE_UNIT || oe.entityType == EntityType::RANGED_UNIT)
                if (dist(wrk.position, oe.position) <= oe.attackRange + 2) {
                    pr = 3;
                    wrkThreat[wrk.id] = oe.position;
                    break;
                }
            if (oe.entityType == EntityType::TURRET)
                if (dist(wrk.position, oe) <= oe.attackRange) {
                    pr = 3;
                    wrkThreat[wrk.id] = oe.position;
                    break;
                }
        }

        wrkList.emplace_back(pr, wrk.id);
    }

    // for (const auto& [id, th] : wrkThreat)
    //     cerr << "worker " << id << " at " << world.entityMap.at(id).position << " threatened by " << th << endl;

    sort(wrkList.begin(), wrkList.end());

    for (const auto& [_, id] : wrkList) {
        const auto& wrk = world.entityMap.at(id);

        if (wrkThreat.count(wrk.id)) {
            const Cell& threat = wrkThreat[wrk.id];
            Cell target(-1, -1);
            forn(w, 4) {
                const Cell nc = wrk.position ^ w;
                if (nc.inside() && !world.hasNonMovable(nc) && dist(nc, threat) > dist(wrk.position, threat)) {
                    target = nc;
                    break;
                }
            }
            // cerr << "hiding target for " << wrk.id << " : " << target << endl;

            if (target.x != -1) {
                vector<Cell> path = {wrk.position, target};
                actions.emplace_back(id, A_HIDE_MOVE, target, -1, Score(135, 0));
                updateAStar(world, path);
                continue;
            }
        }
        
        pair<vector<Cell>, int> pathToGo = getPathToMany(world, wrk.position, dRes);
        // cerr << "path from " << id << " to res:";
        // for (const Cell& c : pathToGo.first) cerr << " " << c;
        // cerr << endl;

        int targetId = world.getIdAt(pathToGo.first.back());
        if (anyToRepair) {
            pair<vector<Cell>, int> pathToRep = getPathToMany(world, wrk.position, dRep);
            // cerr << pathToRep.size() << endl;
            if (pathToRep.second <= pathToGo.second + 2) {
                targetId = world.getIdAt(pathToRep.first.back());
                if (pathToRep.first.size() <= 2) {
                    actions.emplace_back(id, A_REPAIR, NOWHERE, targetId, Score(120, 0));
                } else {
                    actions.emplace_back(id, A_REPAIR_MOVE, pathToRep.first[1], targetId, Score(101, 0));
                }
                updateAStar(world, pathToRep.first);
                continue;
            }
        }

        if (pathToGo.first.size() <= 2) {
            actions.emplace_back(id, A_GATHER, NOWHERE, targetId, Score(120, 0));
        } else {
            actions.emplace_back(id, A_GATHER_MOVE, pathToGo.first[1], targetId, Score(101, 0));
        }
        updateAStar(world, pathToGo.first);
    }
}