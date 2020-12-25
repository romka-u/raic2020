#pragma once

#include "world.h"
#include <queue>

struct QItem {
    int f, h, d, hm;
    Cell c, prev;
};

using MyQueue = priority_queue<QItem>;

bool operator<(const QItem& a, const QItem& b) {
    if (a.f + a.h != b.f + b.h) return a.f + a.h > b.f + b.h;
    if (a.h != b.h) return a.h > b.h;
    if (a.hm != b.hm) return a.hm > b.hm;
    if (a.d != b.d) return a.d > b.d;
    return a.c < b.c;
}

int hManh(const Cell& from, const Cell& to) {
    return dist(from, to);
}

int hMax(const Cell& from, const Cell& to) {
    return max(abs(from.x - to.x), abs(from.y - to.y));
}

const int D = 19;

int um[88][88];
Cell p[88][88];
int uit;
int busyForAStar[80][80][D + 1];
int foreverStuck[80][80];
int moveUsed[80][80][D + 1][4];
int dRes[82][82], dRep[82][82], dTur[82][82], enemyZone[82][82];
int astick;

void goBfs(const World& world, vector<Cell>& q, int d[82][82]) {
    size_t qb = 0;
    while (qb < q.size()) {
        const Cell cur = q[qb++];
        forn(w, 4) {
            const Cell nc = cur ^ w;
            if (nc.inside() && um[nc.x][nc.y] != uit && !world.hasNonMovable(nc)) {
                um[nc.x][nc.y] = uit;
                q.push_back(nc);
                d[nc.x][nc.y] = d[cur.x][cur.y] + 1;
            }
        }
    }
}

void updateD(const World& world, const vector<int>& resToGather) {
    memset(dRes, 0x7f, sizeof(dRes));
    memset(dRep, 0x7f, sizeof(dRep));
    memset(dTur, 0x7f, sizeof(dTur));

    vector<Cell> q;    
    uit++;
    for (int r : resToGather) {
        const Cell& c = world.entityMap.at(r).position;
        dRes[c.x][c.y] = 0;
        q.push_back(c);
        um[c.x][c.y] = uit;
    }

    goBfs(world, q, dRes);
    
    q.clear();
    uit++;
    for (const Entity& b : world.myBuildings) {
        if (b.health < b.maxHealth) {
            forn(x, b.size) forn(y, b.size) {
                const Cell c = b.position + Cell(x, y);
                dRep[c.x][c.y] = 0;
                q.push_back(c);
                um[c.x][c.y] = uit;
            }
        }
    }

    goBfs(world, q, dRep);

    q.clear();
    uit++;
    for (const Entity& b : world.myBuildings) {
        if (b.entityType == EntityType::TURRET) {
            forn(x, b.size) forn(y, b.size) {
                const Cell c = b.position + Cell(x, y);
                dTur[c.x][c.y] = 0;
                q.push_back(c);
                um[c.x][c.y] = uit;
            }
        }
    }

    goBfs(world, q, dTur);

    memset(enemyZone, 0, sizeof(enemyZone));
    for (const auto& e : world.oppEntities) {
        if (e.entityType == EntityType::RANGED_UNIT || e.entityType == EntityType::MELEE_UNIT || e.entityType == EntityType::TURRET) {
            const int Q = (e.entityType != EntityType::TURRET) * 2;
            for (int x = e.position.x - e.attackRange - Q; x <= e.position.x + Q + e.size - 1 + e.attackRange; x++)
                for (int y = e.position.y - e.attackRange - Q; y <= e.position.y + Q + e.size - 1 + e.attackRange; y++) {
                    Cell nc(x, y);
                    if (nc.inside() && dist(nc, e) <= e.attackRange + Q) {
                        enemyZone[nc.x][nc.y]++;
                    }
                }
        }
    }
}

void clearAStar() {
    astick++;
}

vector<Cell> getPathTo(const World& world, const Cell& from, const Cell& to, int throughCost=4) {
    if (from == to) return {from};
    MyQueue queue;
    queue.push(QItem{0, hManh(from, to), 0, hMax(from, to), from});
    uit++;

    while (!queue.empty()) {
        QItem cur = queue.top();
        queue.pop();

        int stepCost = 1;
        if (um[cur.c.x][cur.c.y] != uit) {
            Cell cc = cur.c;
            um[cc.x][cc.y] = uit;
            p[cc.x][cc.y] = cur.prev;

            if (cc == to) {
                vector<Cell> res;
                while (cc != from) {
                    res.push_back(cc);
                    cc = p[cc.x][cc.y];
                }
                res.push_back(from);
                reverse(res.begin(), res.end());
                return res;
            }

            if (world.hasNonMovable(cc)) {
                if (world.hasResourceAt(cc)) {
                    stepCost = throughCost;
                } else {
                    continue;
                }
            }
        } else {
            continue;
        }

        forn(w, 4) {
            Cell nc = cur.c ^ w;
            int nf = cur.f + stepCost;
            if (cur.d < D) {
                if (busyForAStar[nc.x][nc.y][cur.d + 1] == astick) nf += 19;
                // if (foreverStuck[nc.x][nc.y] == astick) nf += 77;
                if (moveUsed[nc.x][nc.y][cur.d + 1][w ^ 2] == astick) nf += 19;
            }
            if (nc.inside()) {
                queue.push(QItem{nf, hManh(nc, to), cur.d + 1, hMax(nc, to), nc, cur.c});
            }
        }
    }

    return {from};
}

pair<vector<Cell>, int> getPathToMany(const World& world, const Cell& from, int d[82][82]) {
    MyQueue queue;
    queue.push(QItem{0, d[from.x][from.y], 0, 0, from});
    uit++;

    while (!queue.empty()) {
        QItem cur = queue.top();
        queue.pop();

        if (um[cur.c.x][cur.c.y] != uit) {
            Cell cc = cur.c;
            um[cc.x][cc.y] = uit;
            p[cc.x][cc.y] = cur.prev;

            if (d[cc.x][cc.y] == 0) {
                vector<Cell> res;
                while (cc != from) {
                    res.push_back(cc);
                    cc = p[cc.x][cc.y];
                }
                res.push_back(from);
                reverse(res.begin(), res.end());
                return {res, cur.f};
            }

            if (world.hasNonMovable(cc)) {
                continue;
            }
        } else {
            continue;
        }

        forn(w, 4) {
            Cell nc = cur.c ^ w;
            int nf = cur.f + 1;
            if (cur.d < D) {
                if (busyForAStar[nc.x][nc.y][cur.d + 1] == astick/* || foreverStuck[nc.x][nc.y] == astick*/) nf += 91;
                if (moveUsed[nc.x][nc.y][cur.d + 1][w ^ 2] == astick) nf += 91;
            }
            nf += enemyZone[nc.x][nc.y] * 160;
            if (nc.inside()) {
                queue.push(QItem{nf, d[nc.x][nc.y], cur.d + 1, 0, nc, cur.c});
            }
        }
    }

    return {{from}, inf};
}

void updateAStar(const World& world, const vector<Cell>& path) {
    assert(!path.empty());
    Cell last = path.front();
    bool stuck = false;
    // cerr << "update A* with path";
    // for (const Cell& c : path) cerr << " " << c;
    // cerr << ", astick = " << astick << endl;

    for (size_t i = 1; i <= D; i++) {
        if (i < path.size() && !stuck) {
            const Cell& c = path[i];
            if (world.hasNonMovable(c)) stuck = true;
            else {
                if (busyForAStar[c.x][c.y][i] == astick) stuck = true;
                else {
                    busyForAStar[c.x][c.y][i] = astick;
                    // cerr << "mark " << c << "@" << i << " as busy\n";
                }
                forn(w, 4) {
                    if ((last ^ w) == c) {
                        if (moveUsed[c.x][c.y][i][w ^ 2] == astick) stuck = true;
                        moveUsed[last.x][last.y][i][w] = astick;
                        // cerr << "mark " << last << "->" << w << "@" << i << " as busy\n";
                        break;
                    }
                }
            }

            if (stuck) {
                busyForAStar[last.x][last.y][i] = astick;
                foreverStuck[last.x][last.y] = astick;
                // cerr << "mark " << last << "@" << i << " as busy\n";
            } else {
                last = c;
            }
        } else {
            busyForAStar[last.x][last.y][i] = astick;
            foreverStuck[last.x][last.y] = astick;
            // cerr << "mark " << last << "@" << i << " as busy\n";
        }        
    }
}