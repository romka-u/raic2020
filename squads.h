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

Cell INITIAL{15, 19};

unordered_set<Cell> groupPoints;

Cell getInitialPoint(const World& world) {
    vector<int> hx(80);
    int py = 0;
    for (int x = 79; x >= 0; x--) {
        int my = 0;
        for (int y = 0; y < 79; y++) {
            const int id = world.eMap[x][y];
            if (id < 0) {
                const Entity& e = world.entityMap.at(-id);
                if (e.playerId == world.myId)
                    my = y;
            }
        }
        if (my < py) my = py;
        if (my > py) py = my;
        hx[x] = my;
    }

    vector<Cell> candidates;
    int ly = hx[0] + 1;
    forn(x, 80) {
        while (ly > hx[x] + 1) {
            candidates.emplace_back(x, ly);
            ly--;
        }
        candidates.emplace_back(x, ly);
        if (hx[x] == 0) break;
    }

    sort(candidates.begin(), candidates.end(), [](const Cell& a, const Cell& b) { return abs(a.x - a.y) < abs(b.x - b.y); });

    for (const Cell& nc : candidates) {
        bool ok = true;

        for (const auto& gp : groupPoints)
            if (dist(gp, nc) <= 8) {
                ok = false;
                break;
            }

        if (ok) {
            forn(dx, 3) forn(dy, 3) {
                Cell qc = nc + Cell(dx, dy);
                if (!qc.inside()) ok = false;
                if (world.hasNonMovable(qc)) ok = false;
            }
        }

        if (ok) {
            groupPoints.insert(nc);
            return nc;
        }
    }

    return INITIAL;
}

SquadInfo createNewSquadInfo(const World& world) {
    SquadInfo si;
    si.unitsAssigned = 0;
    si.unitsAlive = 0;
    si.target = getInitialPoint(world);
    si.together = false;
    si.waiting = 0;
    return si;
}

tuple<Cell, bool, bool> moreOrLessTogether(const vector<Cell>& cells) {
    int minx = inf, maxx = -inf, miny = inf, maxy = -inf;
    for (const auto& c : cells) {
        if (c.x < minx) minx = c.x;
        if (c.x > maxx) maxx = c.x;
        if (c.y < miny) miny = c.y;
        if (c.y > maxy) maxy = c.y;
    }

    const Cell center((minx + maxx) / 2, (miny + maxy) / 2);

    vector<int> p(cells.size());
    forn(i, cells.size()) p[i] = i;

    auto findset = [&p](int x) {
        int y = x;
        while (x != p[x]) x = p[x];
        p[y] = x;
        return x;
    };
    
    forn(i, cells.size())
        forn(j, i) {
            if (dist(cells[i], cells[j]) == 1) {
                int a = findset(i);
                int b = findset(j);
                p[a] = b;
            }
        }

    bool conn0 = true;
    forn(i, cells.size())
        if (findset(i) != findset(0)) {
            conn0 = false;
            break;
        }

    forn(i, cells.size()) p[i] = i;

    forn(i, cells.size())
        forn(j, i) {
            if (dist(cells[i], cells[j]) <= 2) {
                int a = findset(i);
                int b = findset(j);
                p[a] = b;
            }
        }

    bool conn1 = true;
    forn(i, cells.size())
        if (findset(i) != findset(0)) {
            conn1 = false;
            break;
        }

    return {center, conn0, conn1};
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
            /*if (st.underAttack && world.warriors[world.myId].size() < 19) {
                for (int p = 1; p <= 4; p++) {
                    if (p == myId) continue;
                    for (int wi : world.warriors[p]) {
                        const auto& w = world.entityMap.at(wi);
                        for (int bi : world.buildings[myId]) {
                            if (dist(w.position, world.entityMap.at(bi), world.P(bi).size) <= props.at(w.entityType).attack->attackRange + 10) {
                                si.target = w.position;
                                goto out;
                            }
                        }
                    }
                }
                out: continue;
            }*/

            if (cce.first < 2) {
                if (st.ts[cce.first].state == TS_PLANNED) {
                    si.target = st.ts[cce.first].target;
                    continue;
                }
            }

            if (si.unitsAssigned == 5 || world.warriors[world.myId].size() >= 19) {
                auto [weightMass, conn0, conn1] = moreOrLessTogether(cellsBySquad[cce.first]);
                if (!si.together) {
                    if (!conn0 && si.waiting < 42 && world.warriors[world.myId].size() < 17) {
                        // cerr << ">> Squad " << cce.first << " is waiting last friend" << endl;
                        si.waiting++;
                    } else {
                        groupPoints.erase(si.target);
                        si.target = cell;
                        // cerr << ">> Squad " << cce.first << " is now completed!\n   attacking enemy at dist " << d << " at " << cell << endl;
                        si.together = true;
                    }
                } else {
                    si.target = cell;
                    /*if (si.together && conn1) {
                        si.target = cell;
                        // cerr << ">> Squad " << cce.first << " is more or less together\n   attacking enemy at dist " << d << " at " << cell << endl;
                    } else if (conn0) {
                        si.target = cell;
                        si.together = true;
                        // cerr << ">> Squad " << cce.first << " is connected again\n   attacking enemy at dist " << d << " at " << cell << endl;
                    } else {
                        si.target = weightMass;
                        si.together = false;
                        // cerr << ">> Squad " << cce.first << " is not together, group at " << weightMass << endl;
                    }*/
                }
            } else {
                // cerr << ">> Squad " << cce.first << " is not complete, group at " << si.target << endl;
            }
        }
    }
}