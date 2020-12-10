#pragma once
#include "actions.h"

int ru[88][88], rit, rd[88][88];

void resBfs(const World& world) {
    rit++;
    vector<Cell> q;
    size_t qb = 0;
    for (const auto& r : world.resources) {
        if (r.health == r.maxHealth) {
            ru[r.position.x][r.position.y] = rit;
            q.push_back(r.position);
            rd[r.position.x][r.position.y] = 0;
        }
    }

    while (qb < q.size()) {
        Cell cur = q[qb++];
        forn(w, 4) {
            const Cell nc = cur ^ w;
            if (nc.inside() && ru[nc.x][nc.y] != rit) {
                ru[nc.x][nc.y] = rit;
                q.push_back(nc);
                rd[nc.x][nc.y] = rd[cur.x][cur.y] + 1;
            }
        }
    }
}

void addTrainActions(const PlayerView& playerView, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    int gap = st.underAttack ? 0 : 3;
    for (const Entity& bu : world.myBuildings)
        if (!bu.active && bu.entityType == EntityType::HOUSE)
            gap = 0;

    if (st.foodUsed >= st.foodLimit - gap) return;

    resBfs(world);

    const int myWS = world.workers[world.myId].size();
    for (const Entity& bu : world.myBuildings) {
        if (bu.entityType == EntityType::BUILDER_BASE
            && !st.workersLeftToFixTurrets
            && (!needBuildArmy)
            // && (myWS < world.warriors[world.myId].size() || myWS < 42)
            && myWS < min(77, int(st.resToGather.size() * 0.91))) {
            for (Cell bornPlace : nearCells(bu.position, bu.size)) {
                if (world.isEmpty(bornPlace)) {
                    // int aux = bornPlace.x + bornPlace.y;
                    // if (world.tick < 70) aux = -aux;
                    actions.emplace_back(bu.id, A_TRAIN, bornPlace, EntityType::BUILDER_UNIT, Score{50, -rd[bornPlace.x][bornPlace.y]});
                }
            }
        }
        if (bu.entityType == EntityType::RANGED_BASE
            && (playerView.players[playerView.myId - 1].resource > 100 || world.warriors[world.myId].size() < 25)
            && (needBuildArmy || myWS > 50)
            && world.warriors[world.myId].size() < 64) {
            for (Cell bornPlace : nearCells(bu.position, bu.size)) {
                if (world.isEmpty(bornPlace)) {
                    actions.emplace_back(bu.id, A_TRAIN, bornPlace, EntityType::RANGED_UNIT, Score{30, bornPlace.x + bornPlace.y});
                }
            }
        }
        int cntMelee = 0;
        for (const auto& w : world.myWarriors)
            cntMelee += w.entityType == EntityType::MELEE_UNIT;
        if (bu.entityType == EntityType::MELEE_BASE
            && (playerView.players[playerView.myId - 1].resource > 100 || world.warriors[world.myId].size() < 25)
            && cntMelee < world.tick % 50 - 30
            && !needBuildArmy && false
            && !st.underAttack) {
            for (Cell bornPlace : nearCells(bu.position, bu.size)) {
                if (world.isEmpty(bornPlace)) {
                    actions.emplace_back(bu.id, A_TRAIN, bornPlace, EntityType::MELEE_UNIT, Score{20, bornPlace.x + bornPlace.y});
                }
            }
        }
        /*if (bu.entityType == EntityType::MELEE_BASE && playerView.players[world.myId - 1].resource > 100) {
            for (Cell bornPlace : nearCells(bu.position, props.at(bu.entityType).size)) {
                if (world.isEmpty(bornPlace)) {
                    actions.emplace_back(bi, A_TRAIN, bornPlace, EntityType::MELEE_UNIT, Score{10, bornPlace.x + bornPlace.y});
                }
            }
        }*/
    }
}