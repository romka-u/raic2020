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

struct World {
    unordered_map<int, Entity> entityMap;
    vector<int> workers[5];
    vector<int> warriors[5];
    vector<int> buildings[5];
    vector<int> resources;
    vector<Entity> oppEntities;
    int eMap[88][88];

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
        forn(p, 5) {
            workers[p].clear();
            warriors[p].clear();
            buildings[p].clear();
        }
        resources.clear();
        oppEntities.clear();
        memset(eMap, 0, sizeof(eMap));

        for (const auto& e : ent) {
            assert(e.id != 0);
            const int esz = props[e.entityType].size;
            const int valueToFill = props[e.entityType].canMove ? e.id : -e.id;
            forn(dx, esz) forn(dy, esz) eMap[e.position.x + dx][e.position.y + dy] = valueToFill;

            if (e.playerId) {
                int pid = *e.playerId;
                int eid = e.id;
                if (pid != playerView.myId) oppEntities.push_back(e);
                entityMap[eid] = e;
                if (e.entityType == EntityType::BUILDER_UNIT)
                    workers[pid].push_back(eid);
                else if (e.entityType == EntityType::RANGED_UNIT || e.entityType == EntityType::MELEE_UNIT)
                    warriors[pid].push_back(eid);
                else if (e.entityType == EntityType::BUILDER_BASE
                    || e.entityType == EntityType::MELEE_BASE
                    || e.entityType == EntityType::RANGED_BASE
                    || e.entityType == EntityType::HOUSE
                    || e.entityType == EntityType::WALL
                    || e.entityType == EntityType::TURRET) {
                    buildings[pid].push_back(eid);
                }
            } else {
                assert(e.entityType == EntityType::RESOURCE);
                entityMap[e.id] = e;
                resources.push_back(e.id);
            }
        }
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

int dist(const Entity& a, int asz, const Entity& b, int bsz) {
    assert(asz == 1);
    if (asz == 1 && bsz == 1) return dist(a.position, b.position);
    const int distX = distToSeg(a.position.x, b.position.x, b.position.x + bsz - 1);
    const int distY = distToSeg(a.position.y, b.position.y, b.position.y + bsz - 1);
    return distX + distY;
}

int dist(const Cell& c, const Entity& b, int bsz) {
    const int distX = distToSeg(c.x, b.position.x, b.position.x + bsz - 1);
    const int distY = distToSeg(c.y, b.position.y, b.position.y + bsz - 1);
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