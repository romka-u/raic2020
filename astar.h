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

int h(const Cell& from, const Cell& to) {
    return dist(from, to);
}

int hm(const Cell& from, const Cell& to) {
    return max(abs(from.x - to.x), abs(from.y - to.y));
}

const int D = 19;

int um[88][88];
Cell p[88][88];
int uit;
int busyForAStar[80][80][D + 1];
int foreverStuck[80][80];
int moveUsed[80][80][D + 1][4];
int astick;

void clearAStar() {
    astick++;
}

vector<Cell> getPathTo(const World& world, const Cell& from, const Cell& to) {
    if (from == to) return {from};
    MyQueue queue;
    queue.push(QItem{0, h(from, to), 0, hm(from, to), from});
    uit++;

    while (!queue.empty()) {
        QItem cur = queue.top();
        // cerr << "\nextracted " << cur.c << "...";
        queue.pop();

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
                continue;
            }
        } else {
            continue;
        }

        forn(w, 4) {
            Cell nc = cur.c ^ w;
            // cerr << "\n-> nc = " << nc;
            int nf = cur.f + 1;
            if (cur.d < D) {
                if (busyForAStar[nc.x][nc.y][cur.d + 1] == astick/* || foreverStuck[nc.x][nc.y]*/) nf += 7;
                if (moveUsed[nc.x][nc.y][cur.d + 1][w ^ 2] == astick) nf += 7;
            }
            if (nc.inside()) {
                queue.push(QItem{nf, h(nc, to), cur.d + 1, hm(nc, to), nc, cur.c});
            }
        }
    }

    return {from};
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
                // foreverStuck[last.x][last.y] = astick;
                // cerr << "mark " << last << "@" << i << " as busy\n";
            } else {
                last = c;
            }
        } else {
            busyForAStar[last.x][last.y][i] = astick;
            // foreverStuck[last.x][last.y] = astick;
            // cerr << "mark " << last << "@" << i << " as busy\n";
        }        
    }
}