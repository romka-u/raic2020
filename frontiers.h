#pragma once

#include "game_status.h"

unordered_map<int, Cell> frontTarget;
vector<int> myPower; //, myPowerClose;
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
    unordered_set<int> nearBaseFrontIds;

    forn(x, 80) forn(y, 80)
        if (st.ubg[x][y] == st.ubit && world.infMap[x][y].first == world.myId) {
            myFrontIds.insert(st.borderGroup[x][y]);
            const Cell c(x, y);
            for (const auto& b : world.myBuildings)
                if (dist(c, b) <= 3) {
                    nearBaseFrontIds.insert(st.borderGroup[x][y]);
                }
        }

    for (const auto& w : world.myWarriors) {
        Cell borderCell = st.unitsToCell.at(w.id);
        myFronts[st.borderGroup[borderCell.x][borderCell.y]].emplace_back(st.dtg[w.position.x][w.position.y], w.id);
    }

    vector<int> needSupport;
    unordered_set<int> freeWarriors;
    myPower.assign(st.attackersPower.size(), 0);
    // myPowerClose.assign(st.attackersPower.size(), 0);
    for (int fid : nearBaseFrontIds)
        myPower[fid] = -5;
    unordered_map<int, vector<int>> unitsByFront;

    for (auto& [gr, vpp] : myFronts) {
        sort(vpp.begin(), vpp.end());
        int mpw = 0, mpwc = 0;
        size_t i = 0;
        while (mpw < st.attackersPower[gr] && i < vpp.size()) {
            const auto& u = world.entityMap.at(vpp[i].second);
            mpw += min(11, u.health);
            if (st.dtg[u.position.x][u.position.y] <= 8) {
                mpwc += min(11, u.health);
            }
            frontTarget[u.id] = st.unitsToCell.at(u.id);
            // cerr << "[A] front target of " << u.id << " set to " << frontTarget[u.id] << endl;
            if (st.dtg[u.position.x][u.position.y] <= 6) {
                setClosestTarget(u, world.oppEntities);
            }
            unitsByFront[gr].push_back(u.id);
            i++;
        }
        myPower[gr] = mpw;
        // myPowerClose[gr] = mpwc;

        if (mpwc < st.attackersPowerClose[gr] && nearBaseFrontIds.find(gr) == nearBaseFrontIds.end()) {
            for (const auto& [_, id] : vpp) {
                const auto& u = world.entityMap.at(id);
                if (st.dtg[u.position.x][u.position.y] <= 4)
                    frontTarget[id] = Cell(7, 7);
            }
        }
        for (; i < vpp.size(); i++) {
            freeWarriors.insert(vpp[i].second);
        }
    }

    forn(i, st.attackersPower.size())
        if (myPower[i] < st.attackersPower[i] && myFrontIds.count(i)) {
            needSupport.push_back(i);
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
            // cerr << "[B] front target of " << id << " set to " << frontTarget[id] << endl;
            unitsByFront[gr].push_back(id);
            freeWarriors.erase(id);
        }
    }

    for (int freeId : freeWarriors) {
        const Cell c = st.unitsToCell.at(freeId);
        int grId = st.borderGroup[c.x][c.y];
        unitsByFront[grId].push_back(freeId);
        frontTarget[freeId] = c;
        // cerr << "[C] front target of " << freeId << " set to " << frontTarget[freeId] << endl;
        // myPower[st.borderGroup[c.x][c.y]] += 10;
    }

    forn(i, st.attackersPower.size())
        if (myPower[i] == st.attackersPower[i] && myPower[i] == 0 && world.tick < 400 && world.myWarriors.size() <= 10) {
            if (!unitsByFront[i].empty()) {
                Cell center(0, 0);
                const auto& vu = unitsByFront.at(i);
                for (int id : vu) {
                    const auto& u = world.entityMap.at(id);
                    center = center + u.position;
                }
                center = Cell(center.x / vu.size(), center.y / vu.size());
                // cerr << "Front " << i << "; 0 vs 0, center " << center << " of " << vu.size() << " units\n";
                for (int id : vu)
                    frontTarget[id] = center;
            }
        }

    needBuildArmy = false;
    for (int grId : myFrontIds) {
        if (myPower[grId] < st.attackersPower[grId] && st.attackersPowerClose[grId] != 0) {
            needBuildArmy = true;
        }
    }
    /*
    for (const auto& w : world.myWarriors) {
        assert(frontTarget.count(w.id));
        cerr << w.id << "->" << frontTarget[w.id] << endl;
    }
    */
}