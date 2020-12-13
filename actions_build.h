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
    return true;
}

bool goodForHouse(const Cell& c, int sz) {
    if (c.x == 0) return c.y % sz == 1;
    if (c.y == 0) return c.x % sz == 1;
    if (c.x < sz || c.y < sz) return false;
    return (c.x % (sz + 2) == 1 || c.x == 3) && (c.y % (sz + 2) == 1 || c.y == 3);
}

bool goodForTurret(const Cell& c, int sz) {
    if (c.x < 27 && c.y < 27) return false;
    if (c.x % 3 != 0 || c.y % 3 != 0) return false;
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

void addBuildActions(const PlayerView& playerView, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    int myId = playerView.myId;

    Score buildScore{1800 - (st.foodLimit - st.foodUsed) * 10, 0};

    if (playerView.players[playerView.myId - 1].resource < 270 && TURRETS_CHEESE) {
        forn(i, KTS)
            if (st.ts[i].state == TS_PLANNED)
                return;
    }

    int housesInProgress = 0;
    for (const auto& b : world.myBuildings)
        if (b.entityType == EntityType::HOUSE && !b.active)
            housesInProgress++;

    for (const auto& wrk : world.myWorkers) {
        if (st.foodLimit < st.foodUsed + 15 && st.foodLimit < 145 && housesInProgress < (2 - st.underAttack) && st.needRanged != 1) {
            // houses
            const int sz = props.at(EntityType::HOUSE).size;
            for (Cell newPos : nearCells(wrk.position - Cell(sz - 1, sz - 1), sz)) {
                if (canBuild(world, newPos, sz) && (goodForHouse(newPos, sz) || st.needRanged == 0) && safeToBuild(world, newPos, sz, 10)) {
                    // if (newPos.x + newPos.y == 3 && !world.hasNonMovable({0, 0})) continue;
                    buildScore.aux = (newPos.x == 0) * 1000 + (newPos.y == 0) * 1000 - newPos.x - newPos.y;
                    actions.emplace_back(wrk.id, A_BUILD, newPos, EntityType::HOUSE, buildScore);
                }
            }
        }
        if (st.needRanged == 1) {
            // ranged
            const int sz = props.at(EntityType::RANGED_BASE).size;
            for (Cell newPos : nearCells(wrk.position - Cell(sz - 1, sz - 1), sz)) {
                if (canBuild(world, newPos, sz) && safeToBuild(world, newPos, sz, 5)) {
                    buildScore.aux = - newPos.x - newPos.y;
                    actions.emplace_back(wrk.id, A_BUILD, newPos, EntityType::RANGED_BASE, buildScore);
                }
            }
        }
        // turrets
        bool nearRes = false;
        forn(q, 4) {
            const Cell nc = wrk.position ^ q;
            if (nc.inside() && world.hasNonMovable(nc) && world.entityMap.at(-world.eMap[nc.x][nc.y]).entityType == EntityType::RESOURCE) {
                nearRes = true;
                break;
            }
        }
        if (nearRes && st.needRanged == 2) {
            Score buildScore(200);
            const int sz = props.at(EntityType::TURRET).size;
            for (Cell newPos : nearCells(wrk.position - Cell(sz - 1, sz - 1), sz)) {
                if (canBuild(world, newPos, sz) && goodForTurret(newPos, sz) && noTurretAhead(world, newPos) && safeToBuild(world, newPos, sz, 10)) {
                    // if (newPos.x + newPos.y == 3 && !world.hasNonMovable({0, 0})) continue;
                    buildScore.aux = -min(newPos.x, newPos.y);
                    actions.emplace_back(wrk.id, A_BUILD, newPos, EntityType::TURRET, buildScore);
                }
            }
        }
    }
}

void addTurretsActions(const PlayerView& playerView, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    forn(i, KTS) {
        if (!TURRETS_CHEESE) break;
        const Cell home(7, 7);
        if (st.ts[i].state == TS_PLANNED) {
            for (int wi : st.ts[i].repairers) {
                const auto& w = world.entityMap.at(wi);

                bool isEnemyClose = false;
                for (const auto& ou : world.oppEntities) {
                    if (dist(w.position, ou) <= 11 + ou.attackRange) {
                        const int sz = props.at(EntityType::TURRET).size;
                        bool isPlaceToBuild = false;
                        Score buildScore(3000, 0);
                        for (Cell newPos : nearCells(w.position - Cell(sz - 1, sz - 1), sz)) {
                            if (canBuild(world, newPos, sz) && safeToBuild(world, newPos, sz, 7) && dist(newPos, home) > 33) {
                                // if (newPos.x + newPos.y == 3 && !world.hasNonMovable({0, 0})) continue;
                                buildScore.aux = -dist(newPos, st.ts[i].target);
                                actions.emplace_back(wi, A_BUILD, newPos, EntityType::TURRET, buildScore);
                                isPlaceToBuild = true;
                            }
                        }
                        if (!isPlaceToBuild) {
                            actions.emplace_back(wi, A_MOVE, Cell{7, 7}, -1, Score(3000, 0));
                        }
                        isEnemyClose = true;
                        break;
                    }
                }

                if (!isEnemyClose) {
                    actions.emplace_back(wi, A_MOVE, st.ts[i].target, -1, Score(3000, 0));
                }
            }
        }

        if (st.ts[i].state == TS_BUILT) {
            const auto& t = world.entityMap.at(st.ts[i].turretId);
            bool second = false;
            for (int ri : st.ts[i].repairers) {
                bool inDanger = t.health < props.at(t.entityType).maxHealth;
                if (!inDanger) {
                    for (const auto& oe : world.oppEntities) {
                        if (dist(oe.position, t) <= 8) {
                            inDanger = true;
                            break;
                        }
                    }
                }
                if (dist(world.entityMap.at(ri).position, t) == 1 && inDanger) {
                    actions.emplace_back(ri, A_REPAIR, NOWHERE, st.ts[i].turretId, Score(3000, 0));
                } else {
                    Cell target = t.position;
                    if (second) {
                        if (target.y > target.x) {
                            if (world.eMap[target.x][target.y - 1] != 0) {
                                target.x++;
                                if (world.hasNonMovable(Cell(target.x, target.y - 1))) { target.x--; target.y++; }
                            }
                        } else {
                            if (world.eMap[target.x - 1][target.y] != 0) {
                                target.y++;
                                if (world.hasNonMovable(Cell(target.x - 1, target.y))) { target.x++; target.y--; }
                            }
                        }                        
                    }
                    actions.emplace_back(ri, A_REPAIR_MOVE, target, -1, Score(3000, 0));
                }
                second = true;
            }
        }
    }

    vector<Entity> closeToTurrets;
    vector<Cell> myTurretCells;
    for (const auto& t : world.myBuildings) {
        if (t.entityType == EntityType::TURRET)
            myTurretCells.push_back(t.position);
    }
    for (const auto& oe : world.oppEntities) {
        for (const Cell& c : myTurretCells)
            if (dist(c, oe) <= 8) {
                closeToTurrets.push_back(oe);
                break;
            }
    }

    for (const auto& wrk : world.myWorkers) {
        int cld = inf;
        int clt = -1;
        Cell clp;
        for (const auto& t : world.myBuildings) {
            if (t.entityType != EntityType::TURRET) continue;
            int cd = dist(wrk.position, t);
            if (cd < cld) {
                cld = cd;
                clt = t.id;
                clp = t.position;
            }
        }

        if (clt != -1) {
            const auto& deft = world.entityMap.at(clt);
            
            for (const auto& oe : closeToTurrets) {
                const int cd = dist(oe.position, deft);
                if (cd <= 8) {
                    const int mScore = st.resToGather.empty() ? 3030 : -3000;
                    if (dist(wrk.position, oe) == 1) {
                        actions.emplace_back(wrk.id, A_ATTACK, NOWHERE, oe.id, Score(mScore - cd, oe.id));
                    } else {
                        actions.emplace_back(wrk.id, A_MOVE, oe.position, -1, Score(mScore - cd, oe.id));
                    }
                }
            }

            const int mScore = st.resToGather.empty() ? 4000 : -4000;
            if (cld == 1) {
                actions.emplace_back(wrk.id, A_REPAIR, NOWHERE, clt, Score(mScore, 0));
            } else {
                actions.emplace_back(wrk.id, A_REPAIR_MOVE, clp, -1, Score(mScore / 2 - cld, 0));
            }
        }
    }
}