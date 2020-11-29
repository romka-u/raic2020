#pragma once

#include "world.h"

struct QItem {
    int f, h;
    Cell c;
};

bool operator<(const QItem& a, const QItem& b) {
    if (a.f + a.h != b.f + b.h) return a.f + a.h < b.f + b.h;
    return a.h < b.h;
}

int h(const Cell& from, const Cell& to) {
    return dist(from, to);
}

int um[88][88];
int uit;
bool busyForAStar[88][88];

Cell getNextCellTo(const World& world, const Cell& from, const Cell& to) {
    if (from == to) return from;
    set<QItem> queue;
    queue.insert(QItem{0, h(to, from), to});
    uit++;
    um[to.x][to.y] = uit;

    while (!queue.empty()) {
        QItem cur = *queue.begin();
        queue.erase(queue.begin());

        forn(w, 4) {
            Cell nc = cur.c ^ w;
            if (nc == from) {
                if (!busyForAStar[cur.c.x][cur.c.y])
                    return cur.c;
            }
            if (nc.inside() && world.eMap[nc.x][nc.y] >= 0 && um[nc.x][nc.y] != uit) {
                um[nc.x][nc.y] = uit;
                queue.insert(QItem{cur.f + 1, h(nc, from), nc});
            }
        }
    }

    return from;
}