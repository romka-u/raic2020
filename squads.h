#pragma once

#include "common.h"
#include "world.h"
#include "game_status.h"

struct SquadInfo {
    int unitsAssigned;
    int unitsAlive;
    bool together;
    Cell target;
    int waiting;
};

vector<SquadInfo> squadInfo;
unordered_map<int, int> squadId;

SquadInfo createNewSquadInfo(const World& world) {
    SquadInfo si;
    si.unitsAssigned = 0;
    si.unitsAlive = 0;
    // si.target = getInitialPoint(world);
    si.together = false;
    si.waiting = 0;
    return si;
}

Cell getLastFreeCell(const World& world, Cell cur, int d) {
    while (cur.x < 40 && cur.y < 40 && !world.hasNonMovable(cur))
        cur = cur ^ d;
    cur = cur ^ (d ^ 2);
    return cur;
}

void rearrangeSquadsKMeans(const World& world, int K) {
    const auto& ww = world.myWarriors;
    if (ww.size() < K) return;
    vector<Cell> center(K);
    vector<int> cnt(K);
    vector<int> belong(ww.size());
    forn(i, K) {
        center[i] = ww[i].position;
    }

    forn(it, 42) {
        if (it == 41) {
            sort(center.begin(), center.end(), [](const Cell& a, const Cell& b) { return a.x - a.y < b.x - b.y; });
        }
        forn(i, ww.size()) {
            belong[i] = 0;
            const Cell& mp = ww[i].position;
            for (int j = 1; j < K; j++) {
                if (dist(mp, center[j]) < dist(mp, center[belong[i]]))
                    belong[i] = j;
            }
        }

        forn(j, K) { center[j] = {0, 0}; cnt[j] = 0; }
        forn(i, ww.size()) {
            center[belong[i]] += ww[i].position;
            cnt[belong[i]]++;
        }

        forn(j, K) if (cnt[j]) center[j] = Cell{center[j].x / cnt[j], center[j].y / cnt[j]};
    }

    forn(i, ww.size())
        squadId[ww[i].id] = belong[i];
}

void rearrangeSquads(const World& world, int K) {
    const auto& ww = world.myWarriors;
    if (ww.size() < K) return;
    vector<pair<Cell, int>> p(ww.size());
    forn(i, p.size()) p[i] = {ww[i].position, ww[i].id};

    sort(p.begin(), p.end(),
         [](const pair<Cell, int>& a, const pair<Cell, int>& b)
         { return a.first.x - a.first.y < b.first.x - b.first.y; });

    forn(i, p.size())
        squadId[p[i].second] = i * K / p.size();
}

void calcSquadsTactic(const World& world, const GameStatus& st) {
    if (squadInfo.empty()) {
        if (TURRETS_CHEESE) {
            squadInfo.push_back(createNewSquadInfo(world));
            squadInfo.push_back(createNewSquadInfo(world));
            squadInfo[0].target = Cell{11, 16};
            squadInfo[1].target = Cell{16, 11};
        } else {
            squadInfo.push_back(createNewSquadInfo(world));
            squadInfo.push_back(createNewSquadInfo(world));
            squadInfo.push_back(createNewSquadInfo(world));
            squadInfo.push_back(createNewSquadInfo(world));

            squadInfo[3].target = getLastFreeCell(world, Cell(20, 7), 2);
            squadInfo[2].target = getLastFreeCell(world, Cell(20, 17), 2);
            squadInfo[1].target = getLastFreeCell(world, Cell(7, 20), 3);
            squadInfo[0].target = getLastFreeCell(world, Cell(17, 20), 3);
        }
    }

    unordered_map<int, vector<Cell>> cellsBySquad;
    unordered_map<int, pair<Cell, int>> closestEnemy;

    forn(i, 4) squadInfo[i].unitsAssigned = 0;

    rearrangeSquads(world, 4);
    
    for (int wi : world.warriors[world.myId]) {
        if (squadId.find(wi) != squadId.end()) {
            squadInfo[squadId[wi]].unitsAssigned++;
        }
    }
    
    for (int wi : world.warriors[world.myId]) {
        if (squadId.find(wi) == squadId.end()) {
            if (st.ts[0].state == TS_NOT_BUILD && TURRETS_CHEESE) {
                if (squadInfo[0].unitsAssigned <= squadInfo[1].unitsAssigned) {
                    squadInfo[0].unitsAssigned++;
                    squadId[wi] = 0;
                } else {
                    squadInfo[1].unitsAssigned++;
                    squadId[wi] = 1;
                }
            } else {
                int bi = 0;
                forn(i, 4)
                    if (squadInfo[i].unitsAssigned < squadInfo[bi].unitsAssigned)
                        bi = i;

                squadId[wi] = bi;
                squadInfo[squadId[wi]].unitsAssigned++;
            }
        }
        cellsBySquad[squadId[wi]].push_back(world.entityMap.at(wi).position);        
    }

    auto enemiesToAttack = st.attackers.empty() ? world.oppEntities : st.attackers;
    // if (st.attackers.size() == 1) {
    //     cerr << st.attackers.front().position << endl;
    // }

    for (const auto& cbs : cellsBySquad) {
        // cerr << "[] Squad " << cbs.first << " (sz " << cbs.second.size() << "):";
        // for (const auto& c : cbs.second) cerr << " " << c;
        // cerr << endl;
        int cld = inf;
        for (const Cell& c : cbs.second) {
            for (const auto& ou : enemiesToAttack) {
                int cd = dist(c, ou);
                if (ou.entityType == EntityType::TURRET) cd += 100;
                if (cd < cld && int(ou.position.x < ou.position.y) == cbs.first < 2) {
                    cld = cd;
                    closestEnemy[cbs.first] = {ou.position, cld};
                }
            }
        }
        if (cld == inf) {
            for (const Cell& c : cbs.second) {
                for (const auto& ou : world.oppEntities) {
                    int cd = dist(c, ou);
                    if (ou.entityType == EntityType::TURRET) cd += 100;
                    if (cd < cld && int(ou.position.x < ou.position.y) == cbs.first < 2) {
                        cld = cd;
                        closestEnemy[cbs.first] = {ou.position, cld};
                    }
                }
            }
        }
        // cerr << "   cl enemy: " << closestEnemy[cbs.first].first << ", dist " << closestEnemy[cbs.first].second << endl;
    }
    
    // cerr << "attackers.size = " << st.attackers.size() << endl;
    for (const auto& cce : closestEnemy) {
        auto [cell, d] = cce.second;
        auto& si = squadInfo[cce.first];
        if (d < 7 || !st.attackers.empty()) {
            // cerr << "set " << cell << " as target for squad " << cce.first << endl;
            si.target = cell;
            // cerr << ">> Squad " << cce.first << " is close to enemy\n   distance " << d << ", target " << cell << endl;
        } else {
            if (cce.first < 2) {
                if (st.ts[cce.first].state == TS_PLANNED) {
                    si.target = st.ts[cce.first].target;
                    continue;
                }
            }

            if (world.tick > 200 || world.myWarriors.size() >= 8)
                si.target = cell;
        }
    }
}