#pragma once
#include "actions.h"

void addRepairActions(int myId, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    for (const auto& bu : world.myBuildings) {
        if (bu.health < bu.maxHealth || st.turretsInDanger.count(bu.id)) {
            if (bu.maxHealth - bu.health < 444 - world.tick && bu.entityType != EntityType::TURRET) continue;
            for (const auto& wrk : world.myWorkers) {
                int cd = dist(wrk.position, bu);
                if (cd == 1) {
                    actions.emplace_back(wrk.id, A_REPAIR, NOWHERE, bu.id, Score(120 + 8 * (bu.entityType == EntityType::TURRET), -bu.health));
                } else {
                    actions.emplace_back(wrk.id, A_REPAIR_MOVE, bu.position, -1, Score(101 - cd + 16 * (bu.entityType == EntityType::TURRET), -bu.health));
                }
            }
        }
    }
}