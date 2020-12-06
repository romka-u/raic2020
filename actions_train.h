#pragma once
#include "actions.h"

void addTrainActions(const PlayerView& playerView, const World& world, vector<MyAction>& actions, const GameStatus& st) {
    int gap = 3;
    for (const Entity& bu : world.myBuildings)
        if (!bu.active && bu.entityType == EntityType::HOUSE)
            gap = 0;

    if (st.foodUsed >= st.foodLimit - gap) return;
    const int myWS = world.workers[world.myId].size();
    for (const Entity& bu : world.myBuildings) {
        if (bu.entityType == EntityType::BUILDER_BASE
            && !st.workersLeftToFixTurrets
            && (myWS < world.warriors[world.myId].size() || !st.underAttack)
            && (myWS < world.warriors[world.myId].size() || myWS < 42)
            && myWS < min(77, int(st.resToGather.size() * 0.91))) {
            for (Cell bornPlace : nearCells(bu.position, bu.size)) {
                if (world.isEmpty(bornPlace)) {
                    int aux = bornPlace.x + bornPlace.y;
                    if (world.tick < 70) aux = -aux;
                    actions.emplace_back(bu.id, A_TRAIN, bornPlace, EntityType::BUILDER_UNIT, Score{50, aux});
                }
            }
        }
        if (bu.entityType == EntityType::RANGED_BASE) {
            for (Cell bornPlace : nearCells(bu.position, bu.size)) {
                if (world.isEmpty(bornPlace)) {
                    actions.emplace_back(bu.id, A_TRAIN, bornPlace, EntityType::RANGED_UNIT, Score{30, bornPlace.x + bornPlace.y});
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