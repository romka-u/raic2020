#pragma once

#include "game_status.h"

unordered_map<int, Cell> frontTarget;
vector<int> myPower;

void assignTargets(const World& world, const GameStatus& st) {
    unordered_map<int, vector<pii>> myFronts;
    for (const auto& w : world.myWarriors) {
        Cell borderCell = st.unitsToCell.at(w.id);
        myFronts[st.borderGroup[borderCell.x][borderCell.y]].emplace_back(st.dtg[w.position.x][w.position.y], w.id);
    }

    vector<int> needSupport;
    unordered_set<int> freeWarriors;
    myPower.assign(st.attackersPower.size(), 0);
    frontTarget.clear();

    for (auto& [gr, vpp] : myFronts) {
        sort(vpp.begin(), vpp.end());
        int mpw = 0;
        size_t i = 0;
        while (mpw < st.attackersPower[gr] && i < vpp.size()) {
            mpw += 10;
            frontTarget[vpp[i].second] = st.unitsToCell.at(vpp[i].second);
            i++;
        }
        myPower[gr] = mpw;

        if (mpw < st.attackersPower[gr]) {
            needSupport.push_back(gr);
        } else {
            for (; i < vpp.size(); i++) {
                freeWarriors.insert(vpp[i].second);
            }
        }
    }

    vector<pair<int, pii>> cand;
    for (int gr : needSupport)
        for (int id : freeWarriors) {
            cand.emplace_back(dist(st.hotPoints[gr], world.entityMap.at(id).position), pii(gr, id));
        }

    sort(cand.begin(), cand.end());

    for (const auto& [_, p] : cand) {
        const auto& [gr, id] = p;
        if (freeWarriors.find(id) == freeWarriors.end()) continue;
        if (myPower[gr] < st.attackersPower[gr]) {
            myPower[gr] += 10;
            frontTarget[id] = st.hotPoints[gr];
            freeWarriors.erase(id);
        }
    }

    for (int freeId : freeWarriors) {
        const Cell c = st.unitsToCell.at(freeId);
        frontTarget[freeId] = c;
        myPower[st.borderGroup[c.x][c.y]] += 10;
    }
}