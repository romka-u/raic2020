#pragma once
#include "actions.h"


bool hasEnemyInRange(const Entity& e, const vector<Entity>& allEntities) {
    for (const auto& other : allEntities)
        if (other.playerId != -1 && other.playerId != e.playerId)
            if (dist(e.position, other) <= e.attackRange)
                return true;

    return false;
}

void addWarActions(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    // workers
    for (const auto& wrk : world.myWorkers) {
        if (st.ts.state == TS_BUILDING) break;
        for (const auto& ou : world.oppEntities) {
            if (dist(wrk.position, ou) == 1) {
                actions.emplace_back(wrk.id, A_ATTACK, ou.position, ou.id, Score(150, -ou.health * 1e6 + ou.id));
            }
        }
    }

    unordered_set<int> willAttack;

    // warriors
    for (const auto& w : world.myWarriors) {
        if (attackTarget.find(w.id) != attackTarget.end()) {
            const auto& ou = world.entityMap.at(attackTarget[w.id]);
            actions.emplace_back(w.id, A_ATTACK, ou.position, ou.id, Score(222, 0));
            willAttack.insert(w.id);
        } else {
            for (const auto& ou : world.oppEntities) {
                int cd = dist(w.position, ou);
                if (cd <= w.attackRange) {
                    int score = 200;
                    if (ou.entityType == EntityType::MELEE_UNIT && dist(w.position, ou.position) > 1) score = 197;
                    if (ou.entityType == EntityType::BUILDER_UNIT) score = 194;
                    if (ou.entityType == EntityType::TURRET) score = 190;
                    if (ou.entityType == EntityType::HOUSE) score = 188;
                    if (ou.entityType == EntityType::RANGED_BASE || ou.entityType == EntityType::MELEE_BASE || ou.entityType == EntityType::BUILDER_BASE) score = 150;
                    actions.emplace_back(w.id, A_ATTACK, ou.position, ou.id, Score(score, -ou.health * 1e6 + ou.id));
                    // cerr << w.id << "@" << w.position << " can attack " << ou.id << "@" << ou.position << " - score " << actions.back().score << endl;
                    willAttack.insert(w.id);
                }
            }
        }
    }

    for (const auto& w : world.myWarriors) {
        if (willAttack.find(w.id) != willAttack.end()) continue;
        Cell target = frontTarget[w.id];
        int mainScore = 100;
        bool attackingResource = false;
        if (frontMoves.find(w.id) != frontMoves.end() /* && target != HOME*/) {
            target = w.position ^ frontMoves[w.id];
            mainScore = 120;
            if (target == w.position && w.entityType == EntityType::RANGED_UNIT) {
                int cld = inf;
                Entity cl;
                forn(e, 4) {
                    const Cell nc = w.position ^ e;
                    if (nc.inside() && world.eMap[nc.x][nc.y] < 0) {
                        const int rid = -world.eMap[nc.x][nc.y];
                        const Entity& r = world.entityMap.at(rid);
                        if (r.entityType == EntityType::RESOURCE) {
                            actions.emplace_back(w.id, A_ATTACK, nc, rid, Score(123, 0));
                            attackingResource = true;
                            break;
                        }
                    }
                }
            }
        }

        if (!attackingResource) {
            if (st.unitsToCell.find(w.id) == st.unitsToCell.end() && !world.finals) {
                target = Cell(68, 68);
            }
            actions.emplace_back(w.id, A_MOVE, target, -1, Score(mainScore, -dist(w.position, target)));
        }
    }

    for (const auto& t : world.myBuildings) {
        if (t.entityType != EntityType::TURRET || !t.active) continue;

        for (const auto& ou : world.oppEntities) {
            if (dist(ou, t) <= t.attackRange) {
                int score = 200;
                if (ou.entityType == EntityType::MELEE_UNIT && dist(ou.position, t) > 1) score = 197;
                if (ou.entityType == EntityType::BUILDER_UNIT) score = 194;
                if (ou.entityType == EntityType::TURRET) score = 190;
                if (ou.entityType == EntityType::HOUSE) score = 188;
                if (ou.entityType == EntityType::RANGED_BASE || ou.entityType == EntityType::MELEE_BASE || ou.entityType == EntityType::BUILDER_BASE) score = 150;
                actions.emplace_back(t.id, A_ATTACK, ou.position, ou.id, Score(score, -ou.health * 1e6 + ou.id));
            }
        }
    }
}