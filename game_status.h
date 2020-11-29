#pragma once

#include "common.h"
#include "world.h"

struct GameStatus {
    bool underAttack;
    vector<int> resToGather;

    void update(const PlayerView& playerView, const World& world) {
        int myId = playerView.myId;

        underAttack = false;
        for (int p = 1; p <= 4; p++) {
            if (p == myId) continue;
            for (int wi : world.warriors[p]) {
                const auto& w = world.entityMap.at(wi);
                for (int bi : world.buildings[myId]) {
                    if (dist(w.position, world.entityMap.at(bi), world.P(bi).size) <= props.at(w.entityType).attack->attackRange + 10)
                        underAttack = true;
                }
            }
        }

        resToGather.clear();
        for (int ri : world.resources) {
            const auto& res = world.entityMap.at(ri);
            if (world.infMap[res.position.x][res.position.y] != myId) continue;
            // bool side = false;
            // forn(q, 4) {
            //     Cell c = res.position ^ q;
            //     if (!c.inside()) continue;
            //     if (world.hasNonMovable(c)) continue;
            //     side = true;
            //     break;
            // }
            // if (!side) continue;
            resToGather.push_back(ri);
        }
    }
};