#pragma once

#include "common.h"
#include "world.h"

struct GameStatus {
    bool underAttack;

    void update(const PlayerView& playerView, const World& world) {
        int myId = playerView.myId;

        underAttack = false;
        for (int p = 1; p <= 4; p++) {
            if (p == myId) continue;
            for (int wi : world.warriors[p]) {
                const auto& w = world.entityMap.at(wi);
                for (int bi : world.buildings[myId]) {
                    if (dist(w.position, world.entityMap.at(bi), world.P(bi).size) <= props.at(w.entityType).attack->attackRange)
                        underAttack = true;
                }
            }
        }
    }
};