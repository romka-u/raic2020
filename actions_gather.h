#pragma once
#include "actions.h"

void addGatherActions(int myId, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    unordered_set<int> dangerousRes;
    for (int ri : st.resToGather) {
        const auto& r = world.entityMap.at(ri);
        for (int p = 1; p <= 4; p++)
            if (p != myId) {
                for (int wi : world.warriors[p]) {
                    const auto& ou = world.entityMap.at(wi);
                    if (dist(ou.position, r.position) <= ou.attackRange + 2) {
                        dangerousRes.insert(ri);
                        goto out;
                    }
                }
            }
        out:;
    }

    for (const auto& wrk : world.myWorkers) {
        int underAttack = 0;
        Cell threatPos;
        for (int p = 1; p <= 4 && !underAttack; p++) {
            if (p == myId) continue;
            for (int wi : world.warriors[p]) {
                const auto& w = world.entityMap.at(wi);
                if (dist(w.position, wrk.position) <= w.attackRange + 2) {
                    if (underAttack < 1) {
                        underAttack = 1;
                        threatPos = w.position;
                    }
                    if (dist(w.position, wrk.position) <= w.attackRange + 1) {
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
                const Cell nc = wrk.position ^ w;
                if (nc.inside() && world.eMap[nc.x][nc.y] == 0) {
                    hasEmptyNear = true;
                    break;
                }
            }    

            if (!hasEmptyNear) underAttack = 0;
        }

        if (underAttack) {
            Cell hideTarget{wrk.position.x * 2 - threatPos.x, wrk.position.y * 2 - threatPos.y};
            if (hideTarget.x < 0) hideTarget.x = 0;
            if (hideTarget.y < 0) hideTarget.y = 0;
            if (hideTarget.y >= 80) hideTarget.y = 79;
            if (hideTarget.x >= 80) hideTarget.x = 79;

            actions.emplace_back(wrk.id, A_HIDE_MOVE, hideTarget, -1, Score(/*underAttack * 120*/ 110, 0));
        } else {
            // vector<MyAction> ca;
            for (int ri : st.resToGather) {
                const auto& res = world.entityMap.at(ri);
                const int cd = dist(res.position, wrk.position);
                if (cd == 1 || dangerousRes.find(ri) == dangerousRes.end())
                    actions.emplace_back(wrk.id, A_GATHER, res.position, ri, Score(100 - cd, -wrk.id * 1e5 - ri));
            }
            // sort(ca.begin(), ca.end());
            // forn(x, min(10, int(ca.size()))) {
            //     actions.push_back(ca[x]);
            // }
        } 
    }
}