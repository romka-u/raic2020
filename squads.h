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
    for (int s = 24; s <= 160; s++)
        forn(xx, s + 1) {
            const int x = (xx + s / 2) % (s + 1);
            const Cell nc(x, s - x);
            bool ok = true;

            for (const auto& gp : groupPoints)
                if (dist(gp, nc) <= 5) {
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

void calcSquadsTactic(int myId, const World& world, const GameStatus& st) {
    if (squadInfo.empty()) {
        squadInfo.push_back(createNewSquadInfo(world));
    }

    unordered_map<int, vector<Cell>> cellsBySquad;
    unordered_map<int, pair<Cell, int>> closestEnemy;
    for (int wi : world.warriors[myId]) {
        if (squadId.find(wi) == squadId.end()) {
            if (squadInfo.back().unitsAssigned == 5)
                squadInfo.push_back(createNewSquadInfo(world));

            squadInfo.back().unitsAssigned++;
            squadId[wi] = squadInfo.size() - 1;
        }
        cellsBySquad[squadId[wi]].push_back(world.entityMap.at(wi).position);
    }

    for (const auto& cbs : cellsBySquad) {
        cerr << "[] Squad " << cbs.first << ":";
        for (const auto& c : cbs.second) cerr << " " << c;
        cerr << endl;
        int cld = inf;
        for (int p = 1; p <= 4; p++)
            if (p != myId) {
                for (const Cell& c : cbs.second) {
                    for (int oi : world.warriors[p]) {
                        const auto& ou = world.entityMap.at(oi);
                        int cd = dist(c, ou.position);
                        if (cd < cld) {
                            cld = cd;
                            closestEnemy[cbs.first] = {ou.position, cld};
                        }
                    }
                    for (int oi : world.buildings[p]) {
                        const auto& ou = world.entityMap.at(oi);
                        int cd = dist(c, ou, props.at(ou.entityType).size);
                        if (cd < cld) {
                            cld = cd;
                            closestEnemy[cbs.first] = {ou.position, cld};
                        }
                    }
                    for (int oi : world.workers[p]) {
                        const auto& ou = world.entityMap.at(oi);
                        int cd = dist(c, ou.position);
                        if (cd < cld) {
                            cld = cd;
                            closestEnemy[cbs.first] = {ou.position, cld};
                        }
                    }
                }
            }
        }
    
    for (const auto& cce : closestEnemy) {
        auto [cell, d] = cce.second;
        auto& si = squadInfo[cce.first];
        if (d < 7) {
            si.target = cell;
            cerr << ">> Squad " << cce.first << " is close to enemy\n   distance " << d << ", target " << cell << endl;
        } else {
            if (st.underAttack) {
                for (int p = 1; p <= 4; p++) {
                    if (p == myId) continue;
                    for (int wi : world.warriors[p]) {
                        const auto& w = world.entityMap.at(wi);
                        for (int bi : world.buildings[myId]) {
                            if (dist(w.position, world.entityMap.at(bi), world.P(bi).size) <= props.at(w.entityType).attack->attackRange) {
                                si.target = w.position;
                                goto out;
                            }
                        }
                    }
                }
                out: continue;
            }

            if (si.unitsAssigned == 5) {
                auto [weightMass, conn0, conn1] = moreOrLessTogether(cellsBySquad[cce.first]);
                if (!si.together) {
                    if (!conn0 && si.waiting < 50) {
                        cerr << ">> Squad " << cce.first << " is waiting last friend" << endl;
                        si.waiting++;
                    } else {
                        groupPoints.erase(si.target);
                        si.target = cell;
                        cerr << ">> Squad " << cce.first << " is now completed!\n   attacking enemy at dist " << d << " at " << cell << endl;
                        si.together = true;
                    }
                } else {
                    si.target = cell;
                    /*if (si.together && conn1) {
                        si.target = cell;
                        cerr << ">> Squad " << cce.first << " is more or less together\n   attacking enemy at dist " << d << " at " << cell << endl;
                    } else if (conn0) {
                        si.target = cell;
                        si.together = true;
                        cerr << ">> Squad " << cce.first << " is connected again\n   attacking enemy at dist " << d << " at " << cell << endl;
                    } else {
                        si.target = weightMass;
                        si.together = false;
                        cerr << ">> Squad " << cce.first << " is not together, group at " << weightMass << endl;
                    }*/
                }
            } else {
                cerr << ">> Squad " << cce.first << " is not complete, group at " << si.target << endl;
            }
        }
    }
}