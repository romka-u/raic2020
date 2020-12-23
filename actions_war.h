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
                if (movableBonus && hasEnemyInRange(ou, world.allEntities)) {
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
                // cerr << w.id << "@" << w.position << " can attack " << ou.id << "@" << ou.position << " - score " << actions.back().score << endl;
                willAttack.insert(w.id);
            }
        }
    }

    for (const auto& w : world.myWarriors) {
        if (willAttack.find(w.id) != willAttack.end()) continue;
        Cell target = frontTarget[w.id];
        int mainScore = 100;
        if (frontMoves.find(w.id) != frontMoves.end() /* && target != HOME*/) {
            target = w.position ^ frontMoves[w.id];
            mainScore = 120;
        }
        if (st.unitsToCell.find(w.id) == st.unitsToCell.end() && !world.finals) {
            target = Cell(68, 68);
        }
        
        actions.emplace_back(w.id, A_MOVE, target, -1, Score(mainScore, -dist(w.position, target)));
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