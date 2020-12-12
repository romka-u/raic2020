#pragma once
#include "actions.h"


bool hasEnemyInRange(const Entity& e, const vector<Entity>& allEntities) {
    for (const auto& other : allEntities)
        if (other.playerId != -1 && other.playerId != e.playerId)
            if (dist(e.position, other) <= e.attackRange)
                return true;

    return false;
}

void addWarActions(const PlayerView& playerView, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    // workers
    for (const auto& wrk : world.myWorkers) {
        for (const auto& ou : world.oppEntities) {
            if (dist(wrk.position, ou) == 1) {
                actions.emplace_back(wrk.id, A_ATTACK, ou.position, ou.id, Score(150, -ou.health * 1e6 + ou.id));
            }
        }
    }

    unordered_map<int, int> closestDist;
    unordered_map<int, Cell> closestEnemyCell;
    unordered_set<int> willAttack;

    // warriors
    for (const auto& w : world.myWarriors) {
        // Cell target = squadInfo[squadId[w.id]].target;
        Cell target = frontTarget[w.id];
        closestDist[w.id] = inf;

        for (const auto& ou : world.oppEntities) {
            // int movableBonus = ou.entityType == EntityType::RANGED_UNIT || ou.entityType == EntityType::MELEE_UNIT;
            // if (world.staying.count(ou.id) && world.staying.at(ou.id) > 4) movableBonus = 0;
            int movableBonus = 0;
            int cd = dist(w.position, ou);
            if ((ou.entityType == EntityType::RANGED_UNIT || ou.entityType == EntityType::TURRET) && cd < closestDist[w.id]) {
                closestDist[w.id] = cd;
                closestEnemyCell[w.id] = ou.position;
            }
            if (cd <= w.attackRange + movableBonus) {
                if (movableBonus && hasEnemyInRange(ou, playerView.entities)) {
                    movableBonus = 0;
                    if (cd > ou.attackRange) continue;
                }
                int score = 200;
                if (ou.entityType == EntityType::MELEE_UNIT && dist(w.position, ou.position) > 1) score = 197;
                if (ou.entityType == EntityType::BUILDER_UNIT) score = 194;
                if (ou.entityType == EntityType::TURRET) score = 190;
                if (ou.entityType == EntityType::HOUSE) score = 188;
                if (ou.entityType == EntityType::RANGED_BASE || ou.entityType == EntityType::MELEE_BASE || ou.entityType == EntityType::BUILDER_BASE) score = 150;
                actions.emplace_back(w.id, A_ATTACK, ou.position, ou.id, Score(score - movableBonus * 30, -ou.health * 1e6 + ou.id));
                willAttack.insert(w.id);
            }
        }
    }

    for (const auto& w : world.myWarriors) {
        if (willAttack.find(w.id) != willAttack.end()) continue;
        const int myCld = closestDist.at(w.id);

        bool iAmOnFront = myCld < 8 && w.entityType != EntityType::MELEE_UNIT;
        for (const auto& at : st.buildingAttackers)
            if (at.position == closestEnemyCell[w.id]) {
                iAmOnFront = false;
                break;
            }
        if (iAmOnFront) {
            for (int wi : world.warriors[world.myId]) {
                if (wi == w.id) continue;
                const auto& wu = world.entityMap.at(wi);
                if (dist(w.position, wu.position) <= 5)
                    if (closestDist[wi] <= myCld) {
                        iAmOnFront = false;
                        break;
                    }
            }
        }

        if (!iAmOnFront) {
            const Cell target = frontTarget[w.id];
            actions.emplace_back(w.id, A_MOVE, target, -1, Score(100, -dist(w.position, target)));
            continue;
        }

        const Cell target = closestEnemyCell[w.id];
        forn(e, 4) {
            const Cell nc = w.position ^ e;
            if (nc.inside() && !world.hasNonMovable(nc)) {
                if (dist(nc, target) > myCld) {
                    actions.emplace_back(w.id, A_MOVE, nc, -1, Score(110 + (world.eMap[nc.x][nc.y] == 0), 0));
                }
            }
        }
    }

    for (const auto& t : world.myBuildings) {
        if (t.entityType != EntityType::TURRET) continue;

        for (const auto& ou : world.oppEntities) {
            if (dist(ou, t) <= t.attackRange) {
                int score = 200;
                if (ou.entityType == EntityType::MELEE_UNIT && dist(t.position, ou.position) > 1) score = 197;
                if (ou.entityType == EntityType::BUILDER_UNIT) score = 194;
                if (ou.entityType == EntityType::TURRET) score = 190;
                if (ou.entityType == EntityType::HOUSE) score = 188;
                if (ou.entityType == EntityType::RANGED_BASE || ou.entityType == EntityType::MELEE_BASE || ou.entityType == EntityType::BUILDER_BASE) score = 150;
                actions.emplace_back(t.id, A_ATTACK, ou.position, ou.id, Score(score, -ou.health * 1e6 + ou.id));
            }
        }
    }
}