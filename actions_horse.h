#pragma once
#include "actions.h"

const vector<Cell> horseRaw = {
    {1,16},
    {2,12},
    {2,13},
    {2,14},
    {2,15},
    {2,16},
    {3,10},
    {3,11},
    {3,12},
    {3,15},
    {3,16},
    {3,23},
    {3,24},
    {3,25},
    {3,26},
    {4,9},
    {4,15},
    {4,21},
    {4,22},
    {4,23},
    {4,26},
    {4,27},
    {5,8},
    {5,15},
    {5,18},
    {5,19},
    {5,20},
    {5,21},
    {5,25},
    {5,27},
    {5,28},
    {6,3},
    {6,8},
    {6,15},
    {6,16},
    {6,17},
    {6,25},
    {6,26},
    {6,28},
    {7,3},
    {7,4},
    {7,5},
    {7,8},
    {7,13},
    {7,14},
    {7,15},
    {7,27},
    {7,28},
    {8,4},
    {8,6},
    {8,8},
    {8,11},
    {8,12},
    {8,13},
    {8,27},
    {9,4},
    {9,7},
    {9,8},
    {9,9},
    {9,10},
    {9,26},
    {9,27},
    {10,5},
    {10,15},
    {10,16},
    {10,25},
    {10,26},
    {11,6},
    {11,15},
    {11,16},
    {11,24},
    {12,5},
    {12,23},
    {13,5},
    {13,6},
    {13,22},
    {13,23},
    {14,5},
    {14,6},
    {14,22},
    {15,5},
    {15,7},
    {15,21},
    {15,22},
    {16,5},
    {16,7},
    {16,8},
    {16,9},
    {16,10},
    {16,21},
    {17,5},
    {17,6},
    {17,10},
    {17,11},
    {17,12},
    {17,20},
    {17,21},
    {18,6},
    {18,11},
    {18,15},
    {18,16},
    {18,17},
    {18,18},
    {18,19},
    {18,20},
    {19,6},
    {19,10},
    {19,11},
    {19,21},
    {20,6},
    {20,10},
    {20,21},
    {20,22},
    {21,6},
    {21,11},
    {21,12},
    {21,23},
    {21,24},
    {22,6},
    {22,13},
    {22,14},
    {22,15},
    {22,24},
    {22,25},
    {23,7},
    {23,16},
    {23,17},
    {23,25},
    {23,26},
    {23,27},
    {24,7},
    {24,14},
    {24,15},
    {24,27},
    {24,28},
    {24,29},
    {24,30},
    {25,7},
    {25,13},
    {25,14},
    {25,30},
    {26,8},
    {26,14},
    {26,15},
    {26,16},
    {26,17},
    {26,18},
    {26,29},
    {27,8},
    {27,18},
    {27,19},
    {27,28},
    {28,9},
    {28,18},
    {28,26},
    {28,27},
    {29,9},
    {29,17},
    {29,23},
    {29,24},
    {29,25},
    {30,10},
    {30,16},
    {30,17},
    {30,18},
    {30,19},
    {30,20},
    {30,21},
    {30,22},
    {31,10},
    {31,11},
    {31,12},
    {31,13},
    {31,14},
    {31,15},
    {31,16},
    {31,17}
};

unordered_map<int, Cell> horseTargets;

void addHorseActions(const World& world, vector<MyAction>& actions, const GameStatus& st) {
    if (world.tick % 42 == 0) horseTargets.clear();
    if (horseTargets.empty()) {
        const Cell offset(45, 79);
        vector<pair<int, int>> participants;
        vector<Cell> horse;
        for (const Cell& c : horseRaw) {
            Cell rc(offset.x + c.x, offset.y - c.y);
            if (world.myId == 2)
                rc = Cell(124 - rc.x, 125 - rc.y);
            horse.push_back(rc);
        }
        for (const auto& w : world.myWarriors) {
            participants.emplace_back(-w.position.x * 100 - w.position.y, w.id);
        }
        for (const auto& w : world.myWorkers) {
            participants.emplace_back(-w.position.x * 100 - w.position.y, w.id);
        }
        sort(participants.begin(), participants.end());
        sort(horse.begin(), horse.end(), [](const Cell& a, const Cell& b) { return b < a; });

        unordered_set<int> uw;
        forn(i, horse.size()) {
            horseTargets[participants[i].second] = horse[i];
            uw.insert(participants[i].second);
        }
                
        for (const auto& w : world.myWarriors)
            if (uw.find(w.id) == uw.end()) {
                horseTargets[w.id] = Cell(0, 0);
            }
        for (const auto& w : world.myWorkers)
            if (uw.find(w.id) == uw.end()) {
                horseTargets[w.id] = Cell(0, 0);
            }
    }

    for (const auto& [id, c] : horseTargets) {
        actions.emplace_back(id, A_MOVE, c, -1, Score(777));
    }
}