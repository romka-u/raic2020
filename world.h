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

void calcInfMap(const vector<Entity>& entities, int eMap[88][88], pii infMap[88][88]) {
    forn(i, 88) forn(j, 88) infMap[i][j] = {0, 0};
    // memset(infMap, 0, sizeof(infMap));

    vector<pair<Cell, int>> q;
    size_t qb = 0;

    for (const auto& e : entities)
        if (e.playerId != -1) {
            forn(x, e.size) forn(y, e.size) {
                const Cell c = e.position + Cell(x, y);
                q.emplace_back(c, e.playerId);
                infMap[c.x][c.y] = {e.playerId, 0};
                // cerr << e.position << ": " << e.playerId << endl;
            }
        }

    while (qb < q.size()) {
        const auto [pos, val] = q[qb++];
        forn(w, 4) {
            const Cell np = pos ^ w;
            const int d = infMap[np.x][np.y].second;
            if (np.inside() && infMap[np.x][np.y].first == 0) {
                infMap[np.x][np.y] = {val, d + 1};
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
    vector<Entity> oppEntities, allEntities;
    vector<Entity> myBuildings, myWarriors, myWorkers;
    unordered_map<int, int> staying;
    unordered_map<EntityType, int> myUnitsCnt;
    int eMap[88][88];
    pii infMap[88][88];
    int myId, tick;
    bool fow;

    World() {
        fow = false;
    }

    bool isEmpty(const Cell& c) const {
        return eMap[c.x][c.y] == 0;
    }

    bool hasNonMovable(const Cell& c) const {
        return eMap[c.x][c.y] < 0;
    }

    EntityProperties& P(int unitId) const {
        return props.at(entityMap.at(unitId).entityType);
    }

    void updateEntities(int playersCnt, const vector<Entity>& newEntities) {
        unordered_map<int, Entity> newEM;

        vector<Entity> my;
        if (fow) {
            for (const auto& e : newEntities)
                if (e.playerId == myId) {
                    my.push_back(e);
                }
        }

        bool haveEnemy = false;

        for (const auto& [id, e] : entityMap) {
            if (!fow) break;
            if (!props.at(e.entityType).canMove) {
                if (e.playerId != myId && e.playerId != -1) {
                    haveEnemy = true;
                }
                
                bool seeing = false;
                for (const auto& me : my)
                    if (dist(me, e) <= props.at(me.entityType).sightRange) {
                        seeing = true;
                        break;
                    }
                if (!seeing) {
                    newEM[e.id] = e;
                }
            }
        }

        if (tick == 0 && !haveEnemy) {
            fow = true;
            Entity base(123456789, 1, EntityType::BUILDER_BASE, Cell(70, 70), props.at(EntityType::BUILDER_BASE).maxHealth, true);
            base.size = 5;
            if (base.playerId == myId) base.playerId++;
            newEM[base.id] = base;

            if (playersCnt > 2) {
                base.id++;
                base.playerId++;
                if (base.playerId == myId) base.playerId++;
                base.position = Cell(70, 5);
                newEM[base.id] = base;

                base.id++;
                base.playerId++;
                if (base.playerId == myId) base.playerId++;
                base.position = Cell(5, 70);
                newEM[base.id] = base;
            }
        }

        entityMap = std::move(newEM);

        for (const auto& e : newEntities) {
            entityMap[e.id] = e;
        }

        allEntities.clear();
        allEntities.reserve(entityMap.size());
        for (const auto& [_, e] : entityMap) {
            allEntities.push_back(e);
        }
    }

    void update(const PlayerView& playerView) {
        myId = playerView.myId;
        tick = playerView.currentTick;
        cerr << "============== Tick " << tick << "=============\n";
        forn(p, 5) {
            workers[p].clear();
            warriors[p].clear();
            buildings[p].clear();
        }
        resources.clear();
        resourceIds.clear();
        oppEntities.clear();
        updateEntities(playerView.players.size(), playerView.entities);
        myUnitsCnt.clear();
        myWorkers.clear();
        myBuildings.clear();
        myWarriors.clear();
        memset(eMap, 0, sizeof(eMap));

        for (const auto& [eid, e] : entityMap) {
            assert(eid != 0);
            const int esz = props[e.entityType].size;
            const int valueToFill = props[e.entityType].canMove ? eid : -eid;
            forn(dx, esz) forn(dy, esz) eMap[e.position.x + dx][e.position.y + dy] = valueToFill;

            if (e.playerId != -1) {
                int pid = e.playerId;
                if (pid != myId) {
                    oppEntities.push_back(e);
                } else {
                    myUnitsCnt[e.entityType]++;
                }                
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
                resources.push_back(e);
                resourceIds.push_back(eid);
            }
        }

        calcInfMap(allEntities, eMap, infMap);
    }
};