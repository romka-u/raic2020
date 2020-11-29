#pragma once

#include "world.h"
#include "common.h"
#include "squads.h"
#include "game_status.h"

struct Score {
    int main;
    double aux;

    Score() {}
    Score(int m): main(m), aux(0) {}
    Score(int m, int a): main(m), aux(a) {}
    Score(int m, double a): main(m), aux(a) {}
};

bool operator<(const Score& a, const Score& b) {
    if (a.main != b.main) return a.main > b.main;
    return a.aux > b.aux;
}

const int A_BUILD = 1;
const int A_ATTACK = 2;
const int A_MOVE = 3;
const int A_REPAIR = 4;
const int A_REPAIR_MOVE = 5;
const int A_TRAIN = 6;
const int A_GATHER = 7;

struct MyAction {
    int unitId;
    int actionType;
    Cell pos;
    int oid;
    Score score;

    MyAction() {}
    MyAction(int u, int a, Cell p, int o, Score s) {
        unitId = u;
        actionType = a;
        pos = p;
        oid = o;
        score = s;
    }
};

bool operator<(const MyAction& a, const MyAction& b) {
    return a.score < b.score;
}

void addGatherActions(int myId, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    for (int bi : world.workers[myId]) {
        const auto& bu = world.entityMap.at(bi);

        bool underAttack = false;
        Cell threatPos;
        for (int p = 1; p <= 4 && !underAttack; p++) {
            if (p == myId) continue;
            for (int wi : world.warriors[p]) {
                const auto& w = world.entityMap.at(wi);
                if (dist(w.position, bu.position) <= props.at(w.entityType).attack->attackRange + 5) {
                    underAttack = true;
                    threatPos = w.position;
                    break;
                }
            }
        }

        if (underAttack) {
            Cell hideTarget{bu.position.x * 2 - threatPos.x, bu.position.y * 2 - threatPos.y};
            if (hideTarget.x < 0) hideTarget.x = 0;
            if (hideTarget.y < 0) hideTarget.y = 0;
            if (hideTarget.y >= 80) hideTarget.y = 79;
            if (hideTarget.x >= 80) hideTarget.x = 79;

            actions.emplace_back(bi, A_MOVE, hideTarget, -1, Score(300, 0));
        } else {
            for (int ri : st.resToGather) {
                const auto& res = world.entityMap.at(ri);
                int cd = dist(res, 1, bu, 1);
                actions.emplace_back(bi, A_GATHER, res.position, ri, Score(100 - cd, 0));
            }
        } 
    }

    cerr << "After add gather: " << actions.size() << " actions.\n";
}
/*
bool intersects(const Cell& c1, int s1, const Cell& c2, int s2) {
    if (min(c1.x + s1 - 1, c2.x + s2 - 1) < max(c1.x, c2.x)) return false;
    if (min(c1.y + s1 - 1, c2.y + s2 - 1) < max(c1.y, c2.y)) return false;
    return true;
}
*/
bool canBuild(const World& world, const Cell& c, int sz) {
    if (!c.inside()) return false;
    Cell cc = c + Cell(sz - 1, sz - 1);
    if (!cc.inside()) return false;
    forn(dx, sz) forn(dy, sz) {
        if (!world.isEmpty(c + Cell(dx, dy)))
            return false;
    }
    return true;
}

bool goodForHouse(const Cell& c, int sz) {
    if (c.x == 0) return c.y % sz == 0;
    if (c.y == 0) return c.x % sz == 0;
    if (c.x < sz || c.y < sz) return false;
    return c.x % (sz + 2) == 1 && c.y % (sz + 2) == 1;
}

void addBuildActions(const PlayerView& playerView, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    int myId = playerView.myId;
    int food_used = 0, food_limit = 0;
    for (int bi : world.buildings[myId])
        food_limit += world.P(bi).populationProvide;
    for (int bi : world.workers[myId])
        food_used += world.P(bi).populationUse;
    for (int bi : world.warriors[myId])
        food_used += world.P(bi).populationUse;

    Score buildScore{180 - (food_limit - food_used) * 10, 0};

    for (int bi : world.workers[myId]) {
        const auto& bu = world.entityMap.at(bi);

        // houses
        int sz = props.at(EntityType::HOUSE).size;
        for (Cell newPos : nearCells(bu.position - Cell(sz - 1, sz - 1), sz)) {
            if (canBuild(world, newPos, sz) && goodForHouse(newPos, sz)) {
                // buildScore.aux = (newPos.x % (sz + 1) == 1) * 10;
                // buildScore.aux += (newPos.y % (sz + 1) == 1) * 10;
                actions.emplace_back(bi, A_BUILD, newPos, EntityType::HOUSE, buildScore);
            }
        }
    }

    cerr << "After add build: " << actions.size() << " actions.\n";
}

void addTrainActions(int myId, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    for (int bi : world.buildings[myId]) {
        const auto& bu = world.entityMap.at(bi);
        if (bu.entityType == EntityType::BUILDER_BASE && !st.underAttack && world.workers[myId].size() < min(77, int(st.resToGather.size() * 0.91))) {
            for (Cell bornPlace : nearCells(bu.position, props.at(bu.entityType).size)) {
                if (world.isEmpty(bornPlace)) {
                    actions.emplace_back(bi, A_TRAIN, bornPlace, EntityType::BUILDER_UNIT, Score{50, bornPlace.x + bornPlace.y});
                    break;
                }
            }
        }
        if (bu.entityType == EntityType::RANGED_BASE) {
            for (Cell bornPlace : nearCells(bu.position, props.at(bu.entityType).size)) {
                if (world.isEmpty(bornPlace)) {
                    actions.emplace_back(bi, A_TRAIN, bornPlace, EntityType::RANGED_UNIT, Score{30, bornPlace.x + bornPlace.y});
                    break;
                }
            }
        }
    }
    cerr << "After add train: " << actions.size() << " actions.\n";
}

void addRepairActions(int myId, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    for (int bi : world.buildings[myId]) {
        const auto& bu = world.entityMap.at(bi);
        const int bsz = props.at(bu.entityType).size;
        if (bu.health < props.at(bu.entityType).maxHealth) {
            for (int ui : world.workers[myId]) {
                const auto& unit = world.entityMap.at(ui);
                int cd = dist(unit, 1, bu, bsz);
                if (cd == 1) {
                    actions.emplace_back(ui, A_REPAIR, NOWHERE, bi, Score(200 - cd, 0));
                } else {
                    actions.emplace_back(ui, A_REPAIR_MOVE, bu.position, -1, Score(200 - cd, 0));
                }
            }
        }
    }
    cerr << "After add repair: " << actions.size() << " actions.\n";
}

void addWarActions(int myId, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    for (int bi : world.warriors[myId]) {
        const auto& bu = world.entityMap.at(bi);

        Cell target = squadInfo[squadId[bi]].target;
        actions.emplace_back(bi, A_MOVE, target, -1, Score(100, 0));

        const int attackDist = props.at(bu.entityType).attack->attackRange;
        for (const auto& ou : world.oppEntities) {
            const int movableBonus = 0; // ou.entityType == EntityType::RANGED_UNIT || ou.entityType == EntityType::MELEE_UNIT;
            if (dist(bu.position, ou, props.at(ou.entityType).size) <= attackDist + movableBonus) {
                int score = 200;
                if (ou.entityType == EntityType::MELEE_UNIT && dist(bu.position, ou.position) > 1) score = 195;
                if (ou.entityType == EntityType::BUILDER_UNIT) score = 160;
                if (ou.entityType == EntityType::RANGED_BASE || ou.entityType == EntityType::MELEE_BASE || ou.entityType == EntityType::BUILDER_BASE) score = 150;
                actions.emplace_back(bi, A_ATTACK, ou.position, ou.id, Score(score - movableBonus * 30, -ou.health * 1e6 + ou.id));
            }
        }
    }

    for (int bi : world.buildings[myId]) {
        const auto& bu = world.entityMap.at(bi);
        if (bu.entityType != EntityType::TURRET) continue;

        const int attackDist = props.at(bu.entityType).attack->attackRange;
        for (const auto& ou : world.oppEntities) {
            const int movableBonus = ou.entityType == EntityType::RANGED_UNIT || ou.entityType == EntityType::MELEE_UNIT;
            if (dist(ou.position, bu, props.at(bu.entityType).size) <= attackDist + movableBonus) {
                int score = 200;
                if (ou.entityType == EntityType::MELEE_UNIT && dist(bu.position, ou.position) > 1) score = 195;
                if (ou.entityType == EntityType::BUILDER_UNIT) score = 160;
                if (ou.entityType == EntityType::RANGED_BASE || ou.entityType == EntityType::MELEE_BASE || ou.entityType == EntityType::BUILDER_BASE) score = 150;
                actions.emplace_back(bi, A_ATTACK, ou.position, ou.id, Score(score - movableBonus * 30, -ou.health * 1e6 + ou.id));
            }
        }
    }
    cerr << "After add war: " << actions.size() << " actions.\n";
}