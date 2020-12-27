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
    return curReachable >= wasReachable - sz * sz - 2;
}

bool canBuildSimple(const World& world, const Cell& c, int sz) {
    if (!c.inside()) return false;
    Cell cc = c + Cell(sz - 1, sz - 1);
    if (!cc.inside()) return false;
    forn(dx, sz) forn(dy, sz) {
        if (!world.isEmpty(c + Cell(dx, dy)))
            return false;
    }
    return true;
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
    if (c.x < 42 && c.y < 42) return false;
    // if (c.x % 3 != 0 || c.y % 3 != 0) return false;
    return true;
}

bool noTurretAheadAndBehind(const World& world, const Cell& c) {
    if (c.x < c.y) {
        int lx = max(0, c.x - 7);
        int rx = min(79, c.x + 7);
        for (int y = max(c.y - 7, 0); y < 70; y++)
            for (int x = lx; x <= rx; x++) {
                const int id = world.eMap[x][y];
                if (id < 0 && world.entityMap.at(-id).entityType == EntityType::TURRET && world.entityMap.at(-id).playerId == world.myId) {
                    return false;
                }
            }
    } else {
        int ly = max(0, c.y - 7);
        int ry = min(79, c.y + 7);
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

vector<int> wd;

int addBuildRanged(const World& world, vector<MyAction>& actions, const GameStatus& st, bool checkBorder) {
    if (st.needRanged != 1) return -1;

    int bestScore = -inf, bestId = -1;
    Cell bestPos;
    const int sz = props.at(EntityType::RANGED_BASE).size;
    unordered_set<Cell> checkedPos;
    uwit++;
    const int wasReachable = workerCellsReachableUWPrefilled(world);

    Entity ranged;
    ranged.size = sz;
    for (const auto& wrk : world.myWorkers) {
        if (wrk.position.x + wrk.position.y > 80) continue;
        if (usedWorkers.find(wrk.id) != usedWorkers.end()) continue;

        for (Cell newPos : nearCells(wrk.position - Cell(sz - 1, sz - 1), sz)) {
            if (!checkedPos.insert(newPos).second) continue;
            if (canBuild(world, newPos, sz, wasReachable) && safeToBuild(world, newPos, sz, 15)) {
                int score = newPos.x + newPos.y - cellsNear(world, newPos, sz) * 4;
                ranged.position = newPos;
                wd.clear();
                for (const auto& wrk : world.myWorkers) {
                    wd.push_back(dist(wrk.position, ranged));
                }
                sort(wd.begin(), wd.end());
                int lim = min(5, int(wd.size()));
                forn(i, lim) score -= 4 * wd[i];

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
    if (st.needRanged != 2 || true) return -1;

    int bestScore = -inf, bestId = -1;
    Cell bestPos;
    const int sz = props.at(EntityType::TURRET).size;
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
        if (!nearRes) continue;

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
    if (st.ts.state == TS_PLANNED) return -1;

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
    unordered_set<Cell> checkedPos;
    uwit++;
    const int wasReachable = workerCellsReachableUWPrefilled(world);

    Entity house;
    house.size = sz;
    const int MAX_FL = world.finals ? 200 : 145;
    for (const auto& wrk : world.myWorkers) {
        if (st.foodLimit >= st.foodUsed + 10 || st.foodLimit >= MAX_FL || st.needRanged == 1)
            break;
        if (usedWorkers.find(wrk.id) != usedWorkers.end()) continue;
        if (wrk.position.x + wrk.position.y > 80) continue;

        for (Cell newPos : nearCells(wrk.position - Cell(sz - 1, sz - 1), sz)) {
            if (!checkedPos.insert(newPos).second) continue;
            if (canBuild(world, newPos, sz, wasReachable) && safeToBuild(world, newPos, sz, 15)) {
                int score = (newPos.x == 0) * 10 + (newPos.y == 0) * 10 - newPos.x - newPos.y;
                if (newPos.x > 0 && newPos.y > 0) score -= cellsNear(world, newPos, sz) * 4;
                house.position = newPos;
                wd.clear();
                for (const auto& wrk : world.myWorkers) {
                    wd.push_back(dist(wrk.position, house));
                }
                sort(wd.begin(), wd.end());
                int lim = min(5, int(wd.size()));
                forn(i, lim) score -= 4 * wd[i];

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
        if (usedWorkers.find(wrk.id) != usedWorkers.end()) continue;
        const int pr = dRep[wrk.position.x][wrk.position.y];
        if (pr > 64) continue;
        wrkList.emplace_back(pr, wrk.id);
    }

    sort(wrkList.begin(), wrkList.end());
    unordered_map<int, int> repairers;

    for (const auto& [pr, id] : wrkList) {
        const auto& wrk = world.entityMap.at(id);

        bool repairing = false;
        forn(w, 4) {
            const Cell nc = wrk.position ^ w;
            if (nc.inside() && world.eMap[nc.x][nc.y] > 0) {
                const Entity& cand = world.entityMap.at(world.eMap[nc.x][nc.y]);
                if (cand.playerId == world.myId && cand.health == 5) {
                    actions.emplace_back(id, A_REPAIR, NOWHERE, cand.id, Score(240, 0));
                    updateAStar(world, {wrk.position});
                    usedWorkers.insert(id);
                    repairing = true;
                    break;
                }
            }
        }

        if (repairing || pr > 1e5) continue;
        // cerr << "id " << id << " pr " << pr << endl;

        pair<vector<Cell>, int> pathToRep = getPathToMany(world, wrk.position, dRep);
        const Entity& repTarget = world.entityMap.at(world.getIdAt(pathToRep.first.back()));
        int healthToRepair = repTarget.maxHealth - repTarget.health;
        int currentWorkers = repairers[repTarget.id];
        int htrwme = healthToRepair - currentWorkers * (pathToRep.second - 1);
        if (htrwme < 0) htrwme = 0;
        int ticksWithMe = pathToRep.second - 1 + (htrwme + currentWorkers) / (currentWorkers + 1);
        int ticksWithoutMe = currentWorkers > 0 ? (healthToRepair + currentWorkers - 1) / currentWorkers : inf;
        // cerr << "id " << id << ", with me " << ticksWithMe << ", w/o me " << ticksWithoutMe << ", path " << pathToRep.second << ", cw " << currentWorkers << endl;
        if (pathToRep.second == 1 && currentWorkers < healthToRepair)
            ticksWithoutMe = inf;
        if (TURRETS_CHEESE && repTarget.entityType == EntityType::TURRET)
            ticksWithoutMe = inf;

        if (pathToRep.second < 19 && ticksWithMe < ticksWithoutMe) {
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
        for (const auto& oe : st.enemiesCloseToWorkers) {
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
    if (st.ts.state == TS_PLANNED) resources -= props.at(EntityType::RANGED_BASE).cost;

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

vector<Cell> getCellsNearRes(const World& world, const GameStatus& st) {
    unordered_set<Cell> cells;
    for (int ri : st.resToGather) {
        const auto& r = world.entityMap.at(ri);
        forn(w, 4) {
            const Cell nc = r.position ^ w;
            if (nc.inside() && !world.hasNonMovable(nc))
                cells.insert(nc);
        }
    }
    return vector<Cell>(cells.begin(), cells.end());
}

int dw[82][82];
const int MAXD = 42;

void bfsWorker(const World& world, const Cell& p) {
    size_t qb = 0;
    static vector<Cell> q;
    q.clear();
    q.push_back(p);
    dw[p.x][p.y] = 0;
    uit++;
    um[p.x][p.y] = uit;

    while (qb < q.size()) {
        const Cell cur = q[qb++];
        if (dw[cur.x][cur.y] == MAXD) break;
        forn(w, 4) {
            const Cell nc = cur ^ w;
            if (nc.inside() && um[nc.x][nc.y] != uit && !world.hasNonMovable(nc)) {
                um[nc.x][nc.y] = uit;
                q.push_back(nc);
                dw[nc.x][nc.y] = dw[cur.x][cur.y] + 1;
            }
        }
    }
}

vector<vector<pii>> gw;
vector<int> ukw;
int ukwit;
vector<int> mt;

bool kuhn(int v, int lim) {
    if (ukw[v] == ukwit) return false;
    ukw[v] = ukwit;
    for (const auto& [w, to] : gw[v]) {
        if (w > lim) break;
        if (mt[to] == -1 || kuhn(mt[to], lim)) {
            mt[to] = v;
            return true;
        }
    }
    return false;
}

int getPairs(int N, int M, int lim) {
    mt.assign(M, -1);

    int res = 0;
    static vector<int> p, nv;
    p.clear();
    forn(i, N) if (!gw[i].empty()) p.push_back(i);

    forn(L, lim + 1) {
        nv.clear();
        for (int v : p) {
            ukwit++;
            if (kuhn(v, L)) {
                res++;
            } else {
                nv.push_back(v);
            }
        }
        if (nv.empty() || res == M) break;
        p = nv;
    }

    return res;
}

void addKuhnGatherActions(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    vector<Cell> cellsNearRes = getCellsNearRes(world, st);
    const int myWS = world.myWorkers.size();
    const int CN = cellsNearRes.size();
    gw.assign(myWS, vector<pii>());

    int maxD = 0;
    forn(i, myWS) {
        const auto& wrk = world.myWorkers[i];
        if (usedWorkers.count(wrk.id)) continue;
        bfsWorker(world, wrk.position);
        forn(j, CN) {
            const auto& c = cellsNearRes[j];
            int d = dw[c.x][c.y];
            if (um[c.x][c.y] != uit) {
                d = MAXD + dist(wrk.position, c);
            }
            gw[i].emplace_back(d, j);
            maxD = max(maxD, d);
        }
        sort(gw[i].begin(), gw[i].end());
    }

    ukw.assign(myWS, 0);
    int maxPairs = getPairs(myWS, CN, maxD);
    /*int L = 0, R = maxD;
    while (R - L > 1) {
        int M = (L + R) >> 1;
        if (getPairs(myWS, CN, M) < maxPairs) {
            L = M;
        } else {
            R = M;
        }
    }
    if (L == 0 && getPairs(myWS, CN, L) == maxPairs) R = L;

    getPairs(myWS, CN, R);*/

    forn(j, CN) {
        if (mt[j] != -1) {
            const Cell& target = cellsNearRes[j];
            const Entity& wrk = world.myWorkers[mt[j]];
            if (wrk.position == target) {
                forn(w, 4) {
                    const Cell nc = wrk.position ^ w;
                    if (nc.inside() && world.eMap[nc.x][nc.y] < 0) {
                        const auto& r = world.entityMap.at(-world.eMap[nc.x][nc.y]);
                        if (r.entityType == EntityType::RESOURCE) {
                            actions.emplace_back(wrk.id, A_GATHER, NOWHERE, r.id, Score(120, 0));
                            usedWorkers.insert(wrk.id);
                            break;
                        }
                    }
                }
            } else {
                actions.emplace_back(wrk.id, A_MOVE, target, -1, Score(101, 0));
                usedWorkers.insert(wrk.id);
            }
        }
    }
}

struct KC {
    int d, i, j;
    KC(int a, int b, int c): d(a), i(b), j(c) {}
};

bool operator<(const KC& a, const KC& b) {
    return a.d < b.d;
}

int awit, anyw[82][82], awind[82][82], toKuhn[1234], usedCells[7777];

void addKuhnLastGatherActions(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    vector<Cell> cellsNearRes = getCellsNearRes(world, st);
    const int myWS = world.myWorkers.size();
    const int CN = cellsNearRes.size();
    gw.assign(myWS, vector<pii>());

    for (const auto& wrk : world.myWorkers)
        busyKuhn[wrk.position.x][wrk.position.y] = astick;

    int maxD = 0;
    vector<KC> cand;
    awit++;
    forn(i, myWS) {
        const auto& wrk = world.myWorkers[i];
        if (usedWorkers.count(wrk.id)) continue;
        anyw[wrk.position.x][wrk.position.y] = awit;
        awind[wrk.position.x][wrk.position.y] = i;
        bfsWorker(world, wrk.position);
        forn(j, CN) {
            const auto& c = cellsNearRes[j];
            int d = dw[c.x][c.y];
            if (um[c.x][c.y] != uit) {
                d = MAXD + dist(wrk.position, c);
            }
            gw[i].emplace_back(d, j);
            maxD = max(maxD, d);
            cand.push_back(KC(d, i, j));
        }
        sort(gw[i].begin(), gw[i].end());
    }

    sort(cand.begin(), cand.end());
    vector<KC> defined;

    for (const KC& kc : cand) {
        const auto& wrk = world.myWorkers[kc.i];
        if (usedWorkers.count(wrk.id)) continue;
        if (usedCells[kc.j] == awit) continue;
        const Cell& target = cellsNearRes[kc.j];
        // cerr << "look@" << wrk.position << "->" << target << endl;
        vector<Cell> path = getPathTo(world, wrk.position, target, 100);
        bool regroup = false;
        for (int i = 1; i < path.size(); i++) {
            if (i > 1 && !regroup) break;
            const Cell& c = path[i];
            if (anyw[c.x][c.y] == awit) {
                if (!regroup) {
                    toKuhn[kc.i] = awit;
                    regroup = true;
                }
                toKuhn[awind[c.x][c.y]] = awit;
                // cerr << "regroup with " << world.myWorkers[awind[c.x][c.y]].position << endl;
            }
        }
        defined.push_back(kc);
        usedCells[kc.j] = awit;
        usedWorkers.insert(wrk.id);
    }

    for (const KC& kc : defined) {
        if (toKuhn[kc.i] != awit) {
            const auto& wrk = world.myWorkers[kc.i];
            const Cell& target = cellsNearRes[kc.j];
            if (wrk.position == target) {
                forn(w, 4) {
                    const Cell nc = wrk.position ^ w;
                    if (nc.inside() && world.eMap[nc.x][nc.y] < 0) {
                        const auto& r = world.entityMap.at(-world.eMap[nc.x][nc.y]);
                        if (r.entityType == EntityType::RESOURCE) {
                            actions.emplace_back(wrk.id, A_GATHER, NOWHERE, r.id, Score(120, 0));
                            updateAStar(world, {wrk.position});
                            break;
                        }
                    }
                }
            } else {
                actions.emplace_back(wrk.id, A_MOVE, target, -1, Score(101, 0));
            }
        } else {
            // cerr << "cancel defined for " << world.myWorkers[kc.i].position << endl;
            usedWorkers.erase(world.myWorkers[kc.i].id);
            usedCells[kc.j] = awit - 1;
        }
    }

    forn(i, myWS) {
        if (toKuhn[i] != awit) {
            gw[i].clear();
            continue;
        }
        int nn = 0;
        forn(j, gw[i].size()) {
            if (usedCells[gw[i][j].second] != awit) {
                gw[i][nn++] = gw[i][j];
            }
        }
        gw[i].resize(nn);
    }

    ukw.assign(myWS, 0);
    int maxPairs = getPairs(myWS, CN, maxD);
    /*int L = 0, R = maxD;
    while (R - L > 1) {
        int M = (L + R) >> 1;
        if (getPairs(myWS, CN, M) < maxPairs) {
            L = M;
        } else {
            R = M;
        }
    }
    if (L == 0 && getPairs(myWS, CN, L) == maxPairs) R = L;

    getPairs(myWS, CN, R);*/

    forn(j, CN) {
        if (mt[j] != -1) {
            const Cell& target = cellsNearRes[j];
            const Entity& wrk = world.myWorkers[mt[j]];
            // cerr << "kuhn assign " << wrk.position << "->" << target << endl;
            if (wrk.position == target) {
                forn(w, 4) {
                    const Cell nc = wrk.position ^ w;
                    if (nc.inside() && world.eMap[nc.x][nc.y] < 0) {
                        const auto& r = world.entityMap.at(-world.eMap[nc.x][nc.y]);
                        if (r.entityType == EntityType::RESOURCE) {
                            actions.emplace_back(wrk.id, A_GATHER, NOWHERE, r.id, Score(120, 0));
                            usedWorkers.insert(wrk.id);
                            break;
                        }
                    }
                }
            } else {
                actions.emplace_back(wrk.id, A_MOVE, target, -1, Score(101, 0));
                usedWorkers.insert(wrk.id);
            }
        }
    }

    for (const auto& wrk : world.myWorkers)
        busyKuhn[wrk.position.x][wrk.position.y] = astick - 1;
}

void addGatherActions(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    vector<pii> wrkList;
    for (const auto& wrk : world.myWorkers) {
        if (usedWorkers.find(wrk.id) != usedWorkers.end()) continue;
        int pr = dRes[wrk.position.x][wrk.position.y];
        if (pr > 1e5) continue;
        wrkList.emplace_back(pr, wrk.id);
    }

    sort(wrkList.begin(), wrkList.end());
    for (const auto& [_, id] : wrkList) {
        const auto& wrk = world.entityMap.at(id);

        pair<vector<Cell>, int> pathToGo = getPathToMany(world, wrk.position, dRes);
        int targetId = world.getIdAt(pathToGo.first.back());

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

void addAntiCheeseActions(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    if (!world.myWarriors.empty()) return;

    vector<pair<int, pii>> cand;
    for (const auto& wrk : world.myWorkers) {
        for (const auto& oe : world.oppEntities)
            if (oe.id < 1e8) {
                const int cd = dist(wrk.position, oe);
                if (cd <= 19)
                    cand.emplace_back(cd + 100 * oe.size, make_pair(wrk.id, oe.id));
            }
    }
    sort(cand.begin(), cand.end());
    unordered_map<int, int> usedOpp;

    vector<pair<int, pair<int, Cell>>> moves;
    // unordered_set<int> attacking;
    for (const auto& [_, p] : cand) {
        const auto& [wi, oi] = p;
        if (usedWorkers.count(wi)) continue;
        const auto& oe = world.entityMap.at(oi);
        const int needWorkers = 2 * oe.size;
        if (usedOpp[oi] >= needWorkers) continue;
        usedOpp[oi]++;
        usedWorkers.insert(wi);
        const auto& wrk = world.entityMap.at(wi);
        // cerr << "> Anticheese " << wrk.position << "->" << oe.position << endl;
        const int cd = dist(wrk.position, oe);
        if (cd == 1) {
            actions.emplace_back(wi, A_ATTACK, oe.position, oi, Score(1234567, 0));
            // attacking.insert(oi);
        } else {
            moves.emplace_back(cd, make_pair(wi, oe.position));
        }
    }

    const Entity* rangedInProgress = nullptr;
    for (const auto& b : world.myBuildings)
        if (b.entityType == EntityType::RANGED_BASE && !b.active) {
            rangedInProgress = &b;
            break;
        }

    if (!rangedInProgress) {
        for (const auto& [_, p] : moves) {
            actions.emplace_back(p.first, A_MOVE, p.second, -1, Score(1234567, 0));
        }
    } else {
        sort(moves.rbegin(), moves.rend());
        int cntAttackers = 0;
        for (const auto& oe : world.oppEntities)
            if (dist(oe, *rangedInProgress) <= oe.attackRange) {
                cntAttackers++;
            }
        if (cntAttackers >= moves.size()) {
            cntAttackers = max(1, (int)moves.size() - 1);
        }
        forn(i, moves.size()) {
            const int wi = moves[i].second.first;
            const auto& wrk = world.entityMap.at(wi);
            if (i < cntAttackers) {
                if (dist(wrk.position, *rangedInProgress) == 1) {
                    actions.emplace_back(wi, A_REPAIR, rangedInProgress->position, rangedInProgress->id, Score(1234567, 0));
                } else {
                    actions.emplace_back(wi, A_MOVE, rangedInProgress->position, -1, Score(1234567, 0));
                }
            } else {
                actions.emplace_back(wi, A_MOVE, moves[i].second.second, -1, Score(1234567, 0));
            }
        }
    }
}

void addCheeseActions(const World& world, vector<MyAction>& actions, const GameStatus& st, int& resources) {
    int bestId = -1, bestScore = -inf;
    Cell bestPos;
    const int sz = props.at(EntityType::RANGED_BASE).size;
    for (int wi : st.ts.repairers) {
        const auto& wrk = world.entityMap.at(wi);
        if (!usedWorkers.insert(wrk.id).second) continue;
        if (resources < props.at(EntityType::RANGED_BASE).cost) continue;
        if (wrk.position.x + wrk.position.y >= 77)
            for (Cell newPos : nearCells(wrk.position - Cell(sz - 1, sz - 1), sz)) {
                if (canBuildSimple(world, newPos, sz)) {
                    int score = newPos.x + newPos.y;
                    if (score > bestScore) {
                        bestId = wrk.id;
                        bestScore = score;
                        bestPos = newPos;
                    }
                }
            }
    }

    if (bestId != -1) {
        resources -= props.at(EntityType::RANGED_BASE).cost;
        actions.emplace_back(bestId, A_BUILD, bestPos, EntityType::RANGED_BASE, Score(1001, 0));
        for (int wi : st.ts.repairers)
            if (wi != bestId)
                actions.emplace_back(wi, A_MOVE, bestPos, bestId, Score(777, 19));
    } else {
        Cell target(79, 79);
        // if (world.oppEntities.size() > 1) target = Cell(3, 3);
        for (int wi : st.ts.repairers)
            actions.emplace_back(wi, A_MOVE, target, -1, Score(777, 19));
    }
}

bool checkFreeSpaceActions(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    int dBase[82][82], dWrk[82][82];
    memset(dBase, 0, sizeof(dBase));
    memset(dWrk, 0, sizeof(dWrk));
    uit++;
    vector<Cell> q;
    q.emplace_back(77, 77);
    goBfs(world, q, dBase);

    uit++;
    for (const auto& wrk : world.myWorkers)
        um[wrk.position.x][wrk.position.y] = uit;

    q.clear();
    q.emplace_back(77, 77);
    goBfs(world, q, dWrk);

    bool isAllZeros = false;
    for (const auto& wrk : world.myWorkers) {
        if (usedWorkers.count(wrk.id)) continue;
        bool ok = false, any = false, hasRes = false;
        forn(w, 4) {
            const Cell nc = wrk.position ^ w;
            if (nc.inside()) {
                if (!world.hasNonMovable(nc)) {
                    any = true;
                    if (um[nc.x][nc.y] == uit) {
                        ok = true;
                        break;
                    }
                } else {
                    const Entity& e = world.entityMap.at(-world.eMap[nc.x][nc.y]);
                    if (e.playerId == -1) {
                        hasRes = true;
                    }
                }
            }
        }
        if (any && !ok && !hasRes) {
            isAllZeros = true;
            break;
        }
    }

    // vector<pii> cand;
    bool haveProblem = false;
    for (const auto& wrk : world.myWorkers) {
        if (!isAllZeros) break;
        if (usedWorkers.count(wrk.id)) continue;
        bool hasZero = false, hasNonZero = false, hasRes = false;
        forn(w, 4) {
            const Cell nc = wrk.position ^ w;
            if (nc.inside()) {
                if (!world.hasNonMovable(nc)) {
                    if (dBase[nc.x][nc.y] > 0) {
                        if (um[nc.x][nc.y] != uit) { hasZero = true; /*cerr << wrk.id << " zero: " << nc << endl;*/ }
                        if (dWrk[nc.x][nc.y] != 0) { hasNonZero = true; /*cerr << wrk.id << " non zero: " << nc << endl;*/ }
                    }
                } else {
                    const Entity& e = world.entityMap.at(-world.eMap[nc.x][nc.y]);
                    if (e.playerId == -1) {
                        hasRes = true;
                    }
                }
            }
        }

        if (hasZero && hasNonZero && hasRes) {
            // cand.emplace_back(dBase[wrk.position.x][wrk.position.y], wrk.id);
            haveProblem = true;
            break;
        }
    }

    return haveProblem;
}

void addWorkersActions(const World& world, vector<MyAction>& actions, const GameStatus& st, int& resources) {
    usedWorkers.clear();
    addAntiCheeseActions(world, actions, st);
    if (st.ts.state == TS_PLANNED) {
        addCheeseActions(world, actions, st, resources);
    }
    // cerr << "workers: " << world.myWorkers.size() << ", oppEntities: " << world.oppEntities.size() << ", resources: " << world.resources.size() << endl;
    // unsigned tt = elapsed();
    addRepairActions(world, actions, st);
    // cerr << "=W= repair actions: " << (elapsed() - tt) << "ms.\n";

    // tt = elapsed();
    addHideActions(world, actions, st);
    // cerr << "=W= hide actions: " << (elapsed() - tt) << "ms.\n";
    // tt = elapsed();
    addBuildActions(world, actions, st, resources);
    // cerr << "=W= build actions: " << (elapsed() - tt) << "ms.\n";
    // tt = elapsed();
    if (world.tick < 444)
        addKuhnLastGatherActions(world, actions, st);
    else
        addGatherActions(world, actions, st);
    // cerr << "=W= gather actions: " << (elapsed() - tt) << "ms.\n";
    // tt = elapsed();
    addTurretsActions(world, actions, st);
    // cerr << "=W= turrets actions: " << (elapsed() - tt) << "ms.\n";
}