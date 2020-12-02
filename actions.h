#pragma once

#include "world.h"
#include "common.h"
#include "squads.h"
#include "game_status.h"

struct Score {
    int main;
    double aux;

    Score() {}
    Score(int m): main(m), aux(0) {}
    Score(int m, int a): main(m), aux(a) {}
    Score(int m, double a): main(m), aux(a) {}
};

bool operator<(const Score& a, const Score& b) {
    if (a.main != b.main) return a.main > b.main;
    return a.aux > b.aux;
}

const int A_BUILD = 1;
const int A_ATTACK = 2;
const int A_MOVE = 3;
const int A_REPAIR = 4;
const int A_REPAIR_MOVE = 5;
const int A_TRAIN = 6;
const int A_GATHER = 7;
const int A_HIDE_MOVE = 8;

struct MyAction {
    int unitId;
    int actionType;
    Cell pos;
    int oid;
    Score score;

    MyAction() {}
    MyAction(int u, int a, Cell p, int o, Score s) {
        unitId = u;
        actionType = a;
        pos = p;
        oid = o;
        score = s;
    }
};

bool operator<(const MyAction& a, const MyAction& b) {
    return a.score < b.score;
}

void addGatherActions(int myId, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    unordered_set<int> dangerousRes;
    for (int ri : st.resToGather) {
        const auto& r = world.entityMap.at(ri);
        for (int p = 1; p <= 4; p++)
            if (p != myId) {
                for (int wi : world.warriors[p]) {
                    const auto& ou = world.entityMap.at(wi);
                    if (dist(ou.position, r.position) <= props.at(ou.entityType).attack->attackRange + 2) {
                        dangerousRes.insert(ri);
                        goto out;
                    }
                }
            }
        out:;
    }

    for (int bi : world.workers[myId]) {
        const auto& bu = world.entityMap.at(bi);

        int underAttack = 0;
        Cell threatPos;
        for (int p = 1; p <= 4 && !underAttack; p++) {
            if (p == myId) continue;
            for (int wi : world.warriors[p]) {
                const auto& w = world.entityMap.at(wi);
                if (dist(w.position, bu.position) <= props.at(w.entityType).attack->attackRange + 3) {
                    if (underAttack < 1) {
                        underAttack = 1;
                        threatPos = w.position;
                    }
                    if (dist(w.position, bu.position) <= props.at(w.entityType).attack->attackRange + 1) {
                        underAttack = 2;
                        threatPos = w.position;              
                        break;
                    }
                }
            }
        }

        if (underAttack) {
            bool hasEmptyNear = false;
            forn(w, 4) {
                const Cell nc = bu.position ^ w;
                if (nc.inside() && world.eMap[nc.x][nc.y] == 0) {
                    hasEmptyNear = true;
                    break;
                }
            }    

            if (!hasEmptyNear) underAttack = 0;
        }

        if (underAttack) {
            Cell hideTarget{bu.position.x * 2 - threatPos.x, bu.position.y * 2 - threatPos.y};
            if (hideTarget.x < 0) hideTarget.x = 0;
            if (hideTarget.y < 0) hideTarget.y = 0;
            if (hideTarget.y >= 80) hideTarget.y = 79;
            if (hideTarget.x >= 80) hideTarget.x = 79;

            actions.emplace_back(bi, A_HIDE_MOVE, hideTarget, -1, Score(/*underAttack * 120*/ 120, 0));
        } else {
            for (int ri : st.resToGather) {
                const auto& res = world.entityMap.at(ri);
                const int cd = dist(res.position, bu.position);
                if (cd == 1 || dangerousRes.find(ri) == dangerousRes.end())
                    actions.emplace_back(bi, A_GATHER, res.position, ri, Score(100 - cd, -bi * 1e5 - ri));
            }
        } 
    }

    // cerr << "After add gather: " << actions.size() << " actions.\n";
}
/*
bool intersects(const Cell& c1, int s1, const Cell& c2, int s2) {
    if (min(c1.x + s1 - 1, c2.x + s2 - 1) < max(c1.x, c2.x)) return false;
    if (min(c1.y + s1 - 1, c2.y + s2 - 1) < max(c1.y, c2.y)) return false;
    return true;
}
*/
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
    return c.x % (sz + 2) == 1 && c.y % (sz + 2) == 1;
}

bool safeToBuild(const World& world, const Cell& c, int sz, int arb) {
    for (const auto& oe : world.oppEntities) {
        const auto& pr = props.at(oe.entityType);
        if (pr.attack)
            if (dist(oe.position, c, sz) <= pr.attack->attackRange + arb)
                return false;
    }
    return true;
}

void addTurretsActions(const PlayerView& playerView, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    forn(i, KTS) {
        if (!TURRETS_CHEESE) break;
        const Cell home(7, 7);
        if (st.ts[i].state == TS_PLANNED) {
            for (int wi : st.ts[i].repairers) {
                const auto& w = world.entityMap.at(wi);

                bool isEnemyClose = false;
                for (const auto& ou : world.oppEntities)
                    if (dist(w.position, ou) <= 16) {
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

                if (!isEnemyClose) {
                    actions.emplace_back(wi, A_MOVE, st.ts[i].target, -1, Score(3000, 0));
                }
            }
        }

        if (st.ts[i].state == TS_BUILT) {
            const auto& t = world.entityMap.at(st.ts[i].turretId);
            bool second = false;
            for (int ri : st.ts[i].repairers) {
                if (dist(world.entityMap.at(ri).position, t) == 1) {
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
    for (int ti : world.buildings[world.myId]) {
        const auto& t = world.entityMap.at(ti);
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

    for (int ri : world.workers[world.myId]) {
        const auto& r = world.entityMap.at(ri);
        int cld = inf;
        int clt = -1;
        Cell clp;
        for (int ti : world.buildings[world.myId]) {
            const auto& t = world.entityMap.at(ti);
            if (t.entityType != EntityType::TURRET) continue;
            int cd = dist(r.position, t);
            if (cd < cld) {
                cld = cd;
                clt = ti;
                clp = t.position;
            }
        }

        if (clt != -1) {
            const auto& deft = world.entityMap.at(clt);
            const int tsz = props.at(deft.entityType).size;

            for (const auto& oe : closeToTurrets) {
                const int cd = dist(oe.position, deft, tsz);
                if (cd <= 8) {
                    const int mScore = st.resToGather.empty() ? 3030 : -3000;
                    if (dist(r.position, oe) == 1) {
                        actions.emplace_back(ri, A_ATTACK, NOWHERE, oe.id, Score(mScore - cd, oe.id));
                    } else {
                        actions.emplace_back(ri, A_MOVE, oe.position, -1, Score(mScore - cd, oe.id));
                    }
                }
            }

            const int mScore = st.resToGather.empty() ? 4000 : -4000;
            if (cld == 1) {
                actions.emplace_back(ri, A_REPAIR, NOWHERE, clt, Score(mScore, 0));
            } else {
                actions.emplace_back(ri, A_REPAIR_MOVE, clp, -1, Score(mScore / 2 - cld, 0));
            }
        }
    }
}

void addBuildActions(const PlayerView& playerView, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    int myId = playerView.myId;

    Score buildScore{1800 - (st.foodLimit - st.foodUsed) * 10, 0};

    if (playerView.players[playerView.myId - 1].resource < 270) {
        forn(i, KTS)
            if (st.ts[i].state == TS_PLANNED)
                return;
    }

    for (int bi : world.workers[myId]) {
        const auto& bu = world.entityMap.at(bi);

        if (st.foodLimit < st.foodUsed + 15) {
            // houses
            int sz = props.at(EntityType::HOUSE).size;
            for (Cell newPos : nearCells(bu.position - Cell(sz - 1, sz - 1), sz)) {
                if (canBuild(world, newPos, sz) && goodForHouse(newPos, sz) && safeToBuild(world, newPos, sz, 5)) {
                    // if (newPos.x + newPos.y == 3 && !world.hasNonMovable({0, 0})) continue;
                    buildScore.aux = (newPos.x == 0) * 1000 + (newPos.y == 0) * 1000 - newPos.x - newPos.y;
                    actions.emplace_back(bi, A_BUILD, newPos, EntityType::HOUSE, buildScore);
                }
            }
        } else {
            // ranged
            int sz = props.at(EntityType::RANGED_BASE).size;
            for (Cell newPos : nearCells(bu.position - Cell(sz - 1, sz - 1), sz)) {
                if (canBuild(world, newPos, sz) && safeToBuild(world, newPos, sz, 5)) {
                    buildScore.aux = - newPos.x - newPos.y;
                    actions.emplace_back(bi, A_BUILD, newPos, EntityType::RANGED_BASE, buildScore);
                }
            }
        }
    }

    // cerr << "After add build: " << actions.size() << " actions.\n";
}

void addTrainActions(const PlayerView& playerView, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    if (st.foodUsed == st.foodLimit) return;
    cerr << "workers: " << world.workers[world.myId].size()
         << ", warriors: " << world.warriors[world.myId].size()
         << ", rtg: " << st.resToGather.size()
        << ", wltft: " << st.workersLeftToFixTurrets << endl;
    for (int bi : world.buildings[world.myId]) {
        const auto& bu = world.entityMap.at(bi);        
        if (bu.entityType == EntityType::BUILDER_BASE && !st.workersLeftToFixTurrets && world.workers[world.myId].size() < min(128, int(st.resToGather.size() * 0.91))) {
            for (Cell bornPlace : nearCells(bu.position, props.at(bu.entityType).size)) {
                if (world.isEmpty(bornPlace)) {
                    int aux = bornPlace.x + bornPlace.y;
                    if (world.tick < 70) aux = -aux;
                    actions.emplace_back(bi, A_TRAIN, bornPlace, EntityType::BUILDER_UNIT, Score{50, aux});
                }
            }
        }
        if (bu.entityType == EntityType::RANGED_BASE) {
            for (Cell bornPlace : nearCells(bu.position, props.at(bu.entityType).size)) {
                if (world.isEmpty(bornPlace)) {
                    actions.emplace_back(bi, A_TRAIN, bornPlace, EntityType::RANGED_UNIT, Score{30, bornPlace.x + bornPlace.y});
                }
            }
        }
        /*if (bu.entityType == EntityType::MELEE_BASE && playerView.players[world.myId - 1].resource > 100) {
            for (Cell bornPlace : nearCells(bu.position, props.at(bu.entityType).size)) {
                if (world.isEmpty(bornPlace)) {
                    actions.emplace_back(bi, A_TRAIN, bornPlace, EntityType::MELEE_UNIT, Score{10, bornPlace.x + bornPlace.y});
                }
            }
        }*/
    }
    // cerr << "After add train: " << actions.size() << " actions.\n";
}

void addRepairActions(int myId, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    for (int bi : world.buildings[myId]) {
        const auto& bu = world.entityMap.at(bi);
        const int bsz = props.at(bu.entityType).size;
        if (bu.health < props.at(bu.entityType).maxHealth) {
            for (int ui : world.workers[myId]) {
                const auto& unit = world.entityMap.at(ui);
                int cd = dist(unit, 1, bu, bsz);
                if (cd == 1) {
                    actions.emplace_back(ui, A_REPAIR, NOWHERE, bi, Score(200 - cd, 0));
                } else {
                    actions.emplace_back(ui, A_REPAIR_MOVE, bu.position, -1, Score(200 - cd, 0));
                }
            }
        }
    }
    // cerr << "After add repair: " << actions.size() << " actions.\n";
}

bool hasEnemyInRange(const Entity& e, const vector<Entity>& allEntities) {
    const int attackRange = props.at(e.entityType).attack->attackRange;
    for (const auto& other : allEntities)
        if (other.playerId && *other.playerId != *e.playerId)
            if (dist(e.position, other, props.at(other.entityType).size) <= attackRange)
                return true;

    return false;
}

void addWarActions(const PlayerView& playerView, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    // workers
    for (int bi : world.workers[world.myId]) {
        const auto& bu = world.entityMap.at(bi);
        for (const auto& ou : world.oppEntities) {
            const int sz = props.at(ou.entityType).size;
            if (dist(bu.position, ou, sz) == 1) {
                actions.emplace_back(bi, A_ATTACK, ou.position, ou.id, Score(150, -ou.health * 1e6 + ou.id));
            }
        }
    }

    // warriors
    for (int bi : world.warriors[world.myId]) {
        const auto& bu = world.entityMap.at(bi);

        Cell target = squadInfo[squadId[bi]].target;
        if (st.underAttack) {
            int cld = inf;
            for (const auto& oe : world.oppEntities)
                if (oe.entityType == EntityType::RANGED_UNIT || oe.entityType == EntityType::MELEE_UNIT) {
                    int cd = dist(bu.position, oe.position);
                    if (cd < cld) {
                        cld = cd;
                        target = oe.position;
                    }
                }
        }
        actions.emplace_back(bi, A_MOVE, target, -1, Score(100, -dist(bu.position, target)));

        const int attackDist = props.at(bu.entityType).attack->attackRange;
        for (const auto& ou : world.oppEntities) {
            // int movableBonus = ou.entityType == EntityType::RANGED_UNIT || ou.entityType == EntityType::MELEE_UNIT;
            // if (world.staying.count(ou.id) && world.staying.at(ou.id) > 4) movableBonus = 0;
            int movableBonus = 0;
            int cd = dist(bu.position, ou, props.at(ou.entityType).size);
            if (cd <= attackDist + movableBonus) {
                if (movableBonus && hasEnemyInRange(ou, playerView.entities)) {
                    movableBonus = 0;
                    if (cd > attackDist) continue;
                }
                int score = 200;
                if (ou.entityType == EntityType::MELEE_UNIT && dist(bu.position, ou.position) > 1) score = 197;
                if (ou.entityType == EntityType::BUILDER_UNIT) score = 194;
                if (ou.entityType == EntityType::TURRET) score = 190;
                if (ou.entityType == EntityType::HOUSE) score = 188;
                if (ou.entityType == EntityType::RANGED_BASE || ou.entityType == EntityType::MELEE_BASE || ou.entityType == EntityType::BUILDER_BASE) score = 150;
                actions.emplace_back(bi, A_ATTACK, ou.position, ou.id, Score(score - movableBonus * 30, -ou.health * 1e6 + ou.id));
            }
        }
    }

    for (int bi : world.buildings[world.myId]) {
        const auto& bu = world.entityMap.at(bi);
        if (bu.entityType != EntityType::TURRET) continue;

        const int attackDist = props.at(bu.entityType).attack->attackRange;
        for (const auto& ou : world.oppEntities) {
            if (dist(ou, bu) <= attackDist) {
                int score = 200;
                if (ou.entityType == EntityType::MELEE_UNIT && dist(bu.position, ou.position) > 1) score = 197;
                if (ou.entityType == EntityType::BUILDER_UNIT) score = 194;
                if (ou.entityType == EntityType::TURRET) score = 190;
                if (ou.entityType == EntityType::HOUSE) score = 188;
                if (ou.entityType == EntityType::RANGED_BASE || ou.entityType == EntityType::MELEE_BASE || ou.entityType == EntityType::BUILDER_BASE) score = 150;
                actions.emplace_back(bi, A_ATTACK, ou.position, ou.id, Score(score, -ou.health * 1e6 + ou.id));
            }
        }
    }
    // cerr << "After add war: " << actions.size() << " actions.\n";
}