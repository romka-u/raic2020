#pragma once

#include "world.h"
#include "common.h"
#include "game_status.h"
#include "frontiers.h"

const int A_BUILD = 1;
const int A_ATTACK = 2;
const int A_MOVE = 3;
const int A_REPAIR = 4;
const int A_REPAIR_MOVE = 5;
const int A_TRAIN = 6;
const int A_GATHER = 7;
const int A_GATHER_MOVE = 8;
const int A_HIDE_MOVE = 9;
const int A_FREE_MOVE = 10;

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

#ifdef DEBUG
unordered_map<int, PathDebug> pathDebug;
#endif