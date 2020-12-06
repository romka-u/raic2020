#pragma once

#include "common.h"

#include "model/Entity.hpp"
#include "model/PlayerView.hpp"
#include "model/EntityType.hpp"
#include "model/EntityProperties.hpp"

unordered_map<EntityType, EntityProperties> props;

Cell operator^(const Cell& a, int q) {
    return {a.x + dx[q], a.y + dy[q]};
}

Cell NOWHERE{-1, -1};

void calcInfMap(const vector<Entity>& entities, int eMap[88][88], int infMap[88][88]) {
    forn(i, 88) forn(j, 88) infMap[i][j] = 0;
    // memset(infMap, 0, sizeof(infMap));

    vector<pair<Cell, int>> q;
    size_t qb = 0;

    for (const auto& e : entities)
        if (e.entityType == EntityType::RANGED_UNIT || e.entityType == EntityType::MELEE_UNIT || e.entityType == EntityType::BUILDER_UNIT) {
            q.emplace_back(e.position, e.playerId);
            infMap[e.position.x][e.position.y] = e.playerId;
        }

    while (qb < q.size()) {
        const auto [pos, val] = q[qb++];
        forn(w, 4) {
            const Cell np = pos ^ w;
            if (np.inside() && infMap[np.x][np.y] == 0) {
                infMap[np.x][np.y] = val;
                if (eMap[np.x][np.y] >= 0)
                    q.emplace_back(np, val);
            }
        }
    }
}

struct World {
    unordered_map<int, Entity> entityMap;
    vector<int> workers[5];
    vector<int> warriors[5];
    vector<int> buildings[5];
    vector<int> resourceIds;
    vector<Entity> resources;
    vector<Entity> oppEntities;
    vector<Entity> myBuildings, myWarriors, myWorkers;
    unordered_map<int, int> staying;
    unordered_map<EntityType, int> myUnitsCnt;
    int eMap[88][88];
    int infMap[88][88];
    int myId, tick;

    bool isEmpty(const Cell& c) const {
        return eMap[c.x][c.y] == 0;
    }

    bool hasNonMovable(const Cell& c) const {
        return eMap[c.x][c.y] < 0;
    }

    EntityProperties& P(int unitId) const {
        return props.at(entityMap.at(unitId).entityType);
    }

    void update(const PlayerView& playerView) {
        const auto& ent = playerView.entities;
        myId = playerView.myId;
        tick = playerView.currentTick;
        forn(p, 5) {
            workers[p].clear();
            warriors[p].clear();
            buildings[p].clear();
        }
        resources.clear();
        resourceIds.clear();
        oppEntities.clear();
        entityMap.clear(); // turret construction relies on that
        myUnitsCnt.clear();
        myWorkers.clear();
        myBuildings.clear();
        myWarriors.clear();
        memset(eMap, 0, sizeof(eMap));

        for (const auto& e : ent) {
            assert(e.id != 0);
            const int esz = props[e.entityType].size;
            const int valueToFill = props[e.entityType].canMove ? e.id : -e.id;
            forn(dx, esz) forn(dy, esz) eMap[e.position.x + dx][e.position.y + dy] = valueToFill;

            if (e.playerId != -1) {
                int pid = e.playerId;
                int eid = e.id;
                if (pid != playerView.myId) {
                    oppEntities.push_back(e);
                } else {
                    myUnitsCnt[e.entityType]++;
                }
                if (entityMap.find(eid) != entityMap.end()) {
                    if (entityMap[eid].position == e.position) {
                        staying[eid]++;
                    } else {
                        staying[eid] = 0;
                    }
                }
                entityMap[eid] = e;
                if (e.entityType == EntityType::BUILDER_UNIT) {
                    workers[pid].push_back(eid);
                    if (pid == myId) myWorkers.push_back(e);
                } else if (e.entityType == EntityType::RANGED_UNIT || e.entityType == EntityType::MELEE_UNIT) {
                    warriors[pid].push_back(eid);
                    if (pid == myId) myWarriors.push_back(e);
                } else if (e.entityType == EntityType::BUILDER_BASE
                    || e.entityType == EntityType::MELEE_BASE
                    || e.entityType == EntityType::RANGED_BASE
                    || e.entityType == EntityType::HOUSE
                    || e.entityType == EntityType::WALL
                    || e.entityType == EntityType::TURRET) {
                    buildings[pid].push_back(eid);
                    if (pid == myId) myBuildings.push_back(e);
                }
            } else {
                assert(e.entityType == EntityType::RESOURCE);
                entityMap[e.id] = e;
                resources.push_back(e);
                resourceIds.push_back(e.id);
            }
        }

        calcInfMap(playerView.entities, eMap, infMap);
    }
};

int distCell(int x1, int y1, int x2, int y2) {
    return abs(x1 - x2) + abs(y1 - y2);
}

int distToSeg(int x, int L, int R) {
    if (x < L) return L - x;
    if (x > R) return x - R;
    return 0;
}

int distSegs(int L1, int R1, int L2, int R2) {
    if (L1 > R2) return L1 - R2;
    if (L2 > R1) return L2 - R1;
    return 0;
}

int dist(const Cell& c, const Entity& b) {
    const int distX = distToSeg(c.x, b.position.x, b.position.x + b.size - 1);
    const int distY = distToSeg(c.y, b.position.y, b.position.y + b.size - 1);
    return distX + distY;
}

int dist(const Cell& c, const Cell& b, int bsz) {
    const int distX = distToSeg(c.x, b.x, b.x + bsz - 1);
    const int distY = distToSeg(c.y, b.y, b.y + bsz - 1);
    return distX + distY;
}

int dist(const Entity& a, const Entity& b) {
    const int distX = distSegs(a.position.x, a.position.x + a.size - 1, b.position.x, b.position.x + b.size - 1);
    const int distY = distSegs(a.position.y, a.position.y + a.size - 1, b.position.y, b.position.y + b.size - 1);
    return distX + distY;
}

vector<Cell> nearCells(const Cell& c, int sz) {
    vector<Cell> res;
    forn(delta, sz) {
        res.emplace_back(c.x - 1, c.y + delta);
        res.emplace_back(c.x + sz, c.y + delta);
        res.emplace_back(c.x + delta, c.y - 1);
        res.emplace_back(c.x + delta, c.y + sz);
    }
    return res;
}