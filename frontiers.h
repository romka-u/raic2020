#pragma once

#include "game_status.h"

unordered_map<int, Cell> frontTarget;
vector<int> myPower;
unordered_set<int> myFrontIds;
bool needBuildArmy;

void setClosestTarget(const Entity& u, const vector<Entity>& oppEntities) {
    int cld = inf;
    for (const auto& ou : oppEntities) {
        int cd = dist(u.position, ou);
        if (cd < cld) {
            cld = cd;
            frontTarget[u.id] = ou.position;
        }
    }
}

void assignTargets(const World& world, const GameStatus& st) {
    frontTarget.clear();
    if (st.unitsToCell.empty()) {
        for (const auto& w : world.myWarriors) {
            frontTarget[w.id] = w.position;
        }
        return;
    }

    unordered_map<int, vector<pii>> myFronts;
    myFrontIds.clear();

    forn(x, 80) forn(y, 80)
        if (st.ubg[x][y] == st.ubit && world.infMap[x][y].first == world.myId)
            myFrontIds.insert(st.borderGroup[x][y]);

    for (const auto& w : world.myWarriors) {
        Cell borderCell = st.unitsToCell.at(w.id);
        myFronts[st.borderGroup[borderCell.x][borderCell.y]].emplace_back(st.dtg[w.position.x][w.position.y], w.id);
    }

    vector<int> needSupport;
    unordered_set<int> freeWarriors;
    myPower.assign(st.attackersPower.size(), 0);
    unordered_map<int, vector<int>> unitsByFront;

    for (auto& [gr, vpp] : myFronts) {
        sort(vpp.begin(), vpp.end());
        int mpw = 0;
        size_t i = 0;
        while (mpw < st.attackersPower[gr] && i < vpp.size()) {
            mpw += 10;
            const auto& u = world.entityMap.at(vpp[i].second);
            frontTarget[u.id] = st.unitsToCell.at(u.id);
            if (st.dtg[u.position.x][u.position.y] <= 6) {
                setClosestTarget(u, world.oppEntities);
            }
            unitsByFront[gr].push_back(u.id);
            i++;
        }
        myPower[gr] = mpw;

        if (mpw < st.attackersPower[gr]) {
            for (const auto& [_, id] : vpp) frontTarget[id] = Cell(7, 7);
        } else {
            for (; i < vpp.size(); i++) {
                freeWarriors.insert(vpp[i].second);
            }
        }
    }

    forn(i, st.attackersPower.size())
        if (myPower[i] < st.attackersPower[i])
            needSupport.push_back(i);

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
            unitsByFront[gr].push_back(id);
            freeWarriors.erase(id);
        }
    }

    for (int freeId : freeWarriors) {
        const Cell c = st.unitsToCell.at(freeId);
        frontTarget[freeId] = c;
        unitsByFront[st.borderGroup[c.x][c.y]].push_back(freeId);
        myPower[st.borderGroup[c.x][c.y]] += 10;
    }

    needBuildArmy = false;
    for (int grId : myFrontIds) {
        if (myPower[grId] < st.attackersPower[grId]) {
            needBuildArmy = true;
        }
    }
}