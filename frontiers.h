#pragma once

#include "game_status.h"

unordered_map<int, Cell> frontTarget;
unordered_map<int, int> attackTarget;
vector<int> myPower; //, myPowerClose;
unordered_set<int> myFrontIds;
bool needBuildArmy;
unordered_map<int, int> frontMoves;
vector<int> possibleTargets[43210];
int idToInd[43210], iti[43210], uiti;

void setClosestTarget(const Entity& u, const vector<Entity>& oppEntities) {
    int cld = inf;
    for (const auto& ou : oppEntities) {
        int cd = dist(u.position, ou);
        if (cd < cld) {
            cld = cd;
            frontTarget[u.id] = ou.position;
        }
    }
}

Cell getNextTarget(const World& world, const Cell& myPos) {
    for (const auto& b : world.oppEntities) {
        const Cell& c = b.position;
        if (c.x > 40 && c.y < 40)
            return c;
    }
    for (const auto& b : world.oppEntities) {
        const Cell& c = b.position;
        if (c.x < 40 && c.y > 40)
            return c;
    }
    for (const auto& b : world.oppEntities) {
        const Cell& c = b.position;
        if (c.x > 40 && c.y > 40)
            return c;
    }
    int cld = inf;
    Cell res(73, 73);
    for (const auto& b : world.oppEntities) {
        int cd = dist(myPos, b);
        if (cd < cld) {
            cld = cd;
            res = b.position;
        }
    }
    return res;
}

void assignTargets(const World& world, const GameStatus& st) {
    frontTarget.clear();
    if (st.unitsToCell.empty()) {
        for (const auto& w : world.myWarriors) {
            frontTarget[w.id] = w.position;
        }
        return;
    }

    unordered_map<int, vector<Entity>> oppUnitsByGroup;
    for (const auto& [unitId, cell] : st.unitsToCell) {
        const auto& u = world.entityMap.at(unitId);
        if (u.playerId != world.myId) {
            const int grId = st.borderGroup[cell.x][cell.y];
            oppUnitsByGroup[grId].push_back(u);
        }
    }

    unordered_map<int, vector<pii>> myFronts;
    myFrontIds.clear();
    unordered_set<int> nearBaseFrontIds;

    forn(x, 80) forn(y, 80)
        if (st.ubg[x][y] == st.ubit && world.infMap[x][y].first == world.myId) {
            myFrontIds.insert(st.borderGroup[x][y]);
            const Cell c(x, y);
            for (const auto& b : world.myBuildings)
                if (dist(c, b) <= 3) {
                    nearBaseFrontIds.insert(st.borderGroup[x][y]);
                }
        }

    for (const auto& w : world.myWarriors) {
        Cell borderCell = st.unitsToCell.at(w.id);
        myFronts[st.borderGroup[borderCell.x][borderCell.y]].emplace_back(st.dtg[w.position.x][w.position.y], w.id);
    }

    vector<int> needSupport;
    unordered_set<int> freeWarriors;
    myPower.assign(st.attackersPower.size(), 0);
    // myPowerClose.assign(st.attackersPower.size(), 0);
    for (int fid : nearBaseFrontIds)
        myPower[fid] = -7;
    unordered_map<int, vector<int>> unitsByFront;

    for (auto& [gr, vpp] : myFronts) {
        sort(vpp.begin(), vpp.end());
        int& mpw = myPower[gr];
        int mpwc = 0;
        size_t i = 0;
        while (mpw < st.attackersPower[gr] && i < vpp.size()) {
            const auto& u = world.entityMap.at(vpp[i].second);
            mpw += min(11, u.health);
            if (st.dtg[u.position.x][u.position.y] <= 8) {
                mpwc += min(11, u.health);
            }
            frontTarget[u.id] = st.unitsToCell.at(u.id);
            if (st.dtg[u.position.x][u.position.y] <= 6) {
                setClosestTarget(u, world.oppEntities);
            } else {
                setClosestTarget(u, oppUnitsByGroup[gr]);
            }
            // cerr << "[A] front target of " << u.id << " set to " << frontTarget[u.id] << endl;
            unitsByFront[gr].push_back(u.id);
            i++;
        }

        if (mpwc < st.attackersPowerClose[gr] && nearBaseFrontIds.find(gr) == nearBaseFrontIds.end()) {
            for (const auto& [_, id] : vpp) {
                const auto& u = world.entityMap.at(id);
                if (st.dtg[u.position.x][u.position.y] <= 4)
                    frontTarget[id] = HOME;
            }
        }
        for (; i < vpp.size(); i++) {
            freeWarriors.insert(vpp[i].second);
        }
    }

    forn(i, st.attackersPower.size())
        if (myPower[i] < st.attackersPower[i] && myFrontIds.count(i)) {
            needSupport.push_back(i);
        }

    vector<pair<int, pii>> cand;
    for (int gr : needSupport)
        for (int id : freeWarriors) {
            cand.emplace_back(dist(st.hotPoints[gr], world.entityMap.at(id).position), pii(gr, id));
        }

    sort(cand.begin(), cand.end());

    for (const auto& [_, p] : cand) {
        const auto& [gr, id] = p;
        const auto& u = world.entityMap.at(id);
        if (freeWarriors.find(id) == freeWarriors.end()) continue;
        if (myPower[gr] < st.attackersPower[gr]) {
            myPower[gr] += min(11, u.health);
            frontTarget[id] = st.hotPoints[gr];
            setClosestTarget(u, oppUnitsByGroup[gr]);
            // cerr << "[B] front target of " << id << " set to " << frontTarget[id] << endl;
            unitsByFront[gr].push_back(id);
            freeWarriors.erase(id);
        }
    }

    for (int freeId : freeWarriors) {
        const Cell c = st.unitsToCell.at(freeId);
        int grId = st.borderGroup[c.x][c.y];
        // cerr << "cell " << c << ", grId: " << grId << " ";
        unitsByFront[grId].push_back(freeId);
        frontTarget[freeId] = st.hotPoints[grId];
        setClosestTarget(world.entityMap.at(freeId), oppUnitsByGroup[grId]);
        // cerr << "[C] front target of " << freeId << " set to " << frontTarget[freeId] << endl;
        // myPower[st.borderGroup[c.x][c.y]] += 10;
    }

    if (!world.finals) {
        forn(i, st.attackersPower.size())
            if (myPower[i] == st.attackersPower[i] && myPower[i] == 0 && world.tick < 400 && world.myWarriors.size() < 7) {
                if (!unitsByFront[i].empty()) {
                    Cell center(0, 0);
                    const auto& vu = unitsByFront.at(i);
                    for (int id : vu) {
                        const auto& u = world.entityMap.at(id);
                        center = center + u.position;
                    }
                    center = Cell(center.x / vu.size(), center.y / vu.size());
                    // cerr << "Front " << i << "; 0 vs 0, center " << center << " of " << vu.size() << " units\n";
                    for (int id : vu)
                        frontTarget[id] = center;
                }
            }
    }

    needBuildArmy = false;
    if (!st.closeToBaseIds.empty()) {
        for (int grId : myFrontIds) {
            if (myPower[grId] < st.attackersPower[grId] && st.attackersPowerClose[grId] != 0) {
                needBuildArmy = true;
            }
        }
    }
}

int ubf, isShooting[100010];

void fillCanMove(const vector<Entity>& my, const vector<Entity>& opp, vector<bool>& myCanMove) {
    forn(i, my.size()) {
        bool canMove = props.at(my[i].entityType).canMove;
        myCanMove[i] = canMove && isShooting[my[i].id] != ubf;
    }
}

void fillMovesAttack(const World& world, const vector<Entity>& my, const vector<Entity>& opp,
                     vector<int>& myMoves, const vector<bool>& myCanMove, int dir=1) {
    forn(i, my.size()) {
        if (!myCanMove[i]) continue;
        int cld = inf;
        Cell ct;
        for (const auto& oe : opp) {
            int cd = dist(my[i].position, oe);
            if (cd < cld) {
                cld = cd;
                ct = oe.position;
            }
        }

        forn(w, 4) {
            const Cell nc = my[i].position ^ w;
            if (nc.inside() && !world.hasNonMovable(nc) && dist(nc, ct) * dir < dist(my[i].position, ct) * dir) {
                myMoves[i] = w;
                break;
            }
        }
        // cerr << my[i].id << my[i].position << " closest enemy at " << ct << ", so attack move: " << myMoves[i] << " to " << (my[i].position ^ myMoves[i]) << endl;
    }
}

Cell myPos[1010], oppPos[1010];
int myHealth[1010], oppHealth[1010], ohb[1010], resHealth[1010], tmpTarget[1010];
int myTarget[1010];
int upos[82][82], pit;

void doDamage(const vector<Entity>& my, const vector<Entity>& opp,
              const vector<int>& myMoves, const vector<int>& oppMoves,
              Cell myPos[1010], Cell oppPos[1010],
              int myTarget[1010],
              int oppHealth[1010]) {
    forn(i, my.size()) {
        const int j = myTarget[i];
        if (j == -1) continue;
        oppHealth[j] = max(0, oppHealth[j] - 5);
    }
}

int getKillScore(const vector<Entity>& my, int myHealth[1010]) {
    int res = 0;
    forn(i, my.size()) {
        res += my[i].health - myHealth[i];
        if (myHealth[i] <= 0) res += 10;
    }
    return res;
}

vector<pair<Cell, int>> rpt[1010];

void doDamageRpt(const vector<Entity>& my, const vector<Entity>& opp,
                 int myTarget[1010], int oppHealth[1010]) {
    forn(i, my.size()) {
        myTarget[i] = -1;
        for (const auto& [_, j] : rpt[i])
            if (oppHealth[j] > 0) {
                oppHealth[j] = max(0, oppHealth[j] - 5);
                myTarget[i] = j;
                break;
            }
    }
}

void doAttack(const vector<Entity>& my, const vector<Entity>& opp,
              const vector<int>& myMoves, const vector<int>& oppMoves,
              Cell myPos[1010], Cell oppPos[1010],
              int oppHealth[1010], bool verbose, bool withRec=false) {
    uiti++;
    forn(i, opp.size()) { idToInd[opp[i].id] = i; iti[opp[i].id] = uiti; }
    forn(i, my.size()) {
        rpt[i].clear();
        for (int tid : possibleTargets[my[i].id]) {
            if (iti[tid] != uiti) continue;
            int j = idToInd[tid];
            if (myMoves[i] != 4) {
                if (oppMoves[j] == 4) {
                    if (dist(myPos[i], opp[j]) <= my[i].attackRange) {
                        rpt[i].emplace_back(oppPos[j], j);
                    }
                } else {
                    if (dist(myPos[i], oppPos[j]) <= my[i].attackRange) {
                        rpt[i].emplace_back(oppPos[j], j);
                    }
                }
            } else {
                if (oppMoves[j] == 4) {
                    if (dist(my[i], opp[j]) <= my[i].attackRange) {
                        rpt[i].emplace_back(oppPos[j], j);
                    }
                } else {
                    if (dist(oppPos[j], my[i]) <= my[i].attackRange) {
                        rpt[i].emplace_back(oppPos[j], j);
                    }
                }
            }
        }
    }

    int best = -inf, score;
    // if (verbose) cerr << "begin check\n";
    auto f = [&]() {
        forn(q, opp.size()) ohb[q] = oppHealth[q];
        doDamageRpt(my, opp, tmpTarget, ohb);
        score = getKillScore(opp, ohb);
        // if (verbose) cerr << "kill score in f: " << score << "\n";
        if (score > best) {
            best = score;
            forn(i, my.size()) myTarget[i] = tmpTarget[i];
            forn(i, opp.size()) resHealth[i] = ohb[i];
        }
    };

    forn(i, my.size())
        sort(rpt[i].begin(), rpt[i].end());
    f();

    forn(i, my.size())
        reverse(rpt[i].begin(), rpt[i].end());
    f();

    forn(i, my.size())
        forn(j, rpt[i].size()) {
            rpt[i][j].first.x = oppHealth[rpt[i][j].second] * 100;
        }

    forn(i, my.size())
        sort(rpt[i].begin(), rpt[i].end());
    f();
    forn(i, my.size())
        reverse(rpt[i].begin(), rpt[i].end());
    f();

    forn(i, my.size())
        forn(j, rpt[i].size()) {
            rpt[i][j].first.x = possibleTargets[opp[rpt[i][j].second].id].size();
        }

    forn(i, my.size())
        sort(rpt[i].rbegin(), rpt[i].rend());
    f();

    forn(i, opp.size()) oppHealth[i] = resHealth[i];
}

int getPosScore(const vector<Entity>& my, Cell myPos[1010]) {
    int res = 0;
    forn(i, my.size()) res += myPos[i].x + myPos[i].y;
    return res;
}

int getSumHealth(const vector<Entity>& my, int myHealth[1010]) {
    int res = 0;
    forn(i, my.size()) res += myHealth[i];
    return res;
}

Score getScore(const vector<Entity>& my, const vector<Entity>& opp,
               const vector<int>& myMoves, const vector<int>& oppMoves,
               bool isFinals, bool verbose=false) {
    pit++;
    forn(i, my.size()) {
        myPos[i] = my[i].position ^ myMoves[i];
        myHealth[i] = my[i].health;
        if (myMoves[i] == 4) upos[myPos[i].x][myPos[i].y] = pit;
    }
    forn(i, opp.size()) {
        oppPos[i] = opp[i].position ^ oppMoves[i];
        oppHealth[i] = opp[i].health;
        if (oppMoves[i] == 4) upos[oppPos[i].x][oppPos[i].y] = pit;
    }

    forn(it, 2) {
        forn(i, my.size()) {
            if (upos[myPos[i].x][myPos[i].y] == pit) {
                myPos[i] = my[i].position;
                upos[myPos[i].x][myPos[i].y] = pit;
            }
        }
        forn(i, opp.size()) {
            if (upos[oppPos[i].x][oppPos[i].y] == pit) {
                oppPos[i] = opp[i].position;
                upos[oppPos[i].x][oppPos[i].y] = pit;
            }
        }
    }

    forn(i, my.size()) {
        if (upos[myPos[i].x][myPos[i].y] == pit) {
            myPos[i] = my[i].position;
        }
        upos[myPos[i].x][myPos[i].y] = pit;
    }
    forn(i, opp.size()) {
        if (upos[oppPos[i].x][oppPos[i].y] == pit) {
            oppPos[i] = opp[i].position;
        }
        upos[oppPos[i].x][oppPos[i].y] = pit;
    }

    doAttack(my, opp, myMoves, oppMoves, myPos, oppPos, oppHealth, false);
    doAttack(opp, my, oppMoves, myMoves, oppPos, myPos, myHealth, false);
    if (verbose) {
        cerr << "\nmy"; forn(i, my.size()) cerr << "|" << myHealth[i];
        cerr << " - opp"; forn(i, opp.size()) cerr << "|" << oppHealth[i];
        cerr << "\n";
    }

    const int sumMyHealth = getSumHealth(my, myHealth);
    const int sumOppHealth = getSumHealth(opp, oppHealth);

    const int MY_KILL_COEFF = (isFinals && sumMyHealth <= sumOppHealth) ? 11 : 10;
    if (verbose) {
        cerr << getKillScore(opp, oppHealth) << "*10 - " << getKillScore(my, myHealth) << "*11 =";
    }
    Score res(getKillScore(opp, oppHealth) * 10 - getKillScore(my, myHealth) * MY_KILL_COEFF, 0);
    forn(i, my.size()) {
        const Cell& cur = myPos[i];
        int cld = inf;
        forn(j, opp.size()) {
            int cd = dist(cur, oppPos[j]);
            if (cd < cld) {
                cld = cd;
            }
        }
        res.aux -= cld;
    }

    for (const auto& oe : opp)
        if (oe.entityType == EntityType::TURRET && oe.active) {
            int cntInRange = 0;
            int shots = 0;
            forn(i, my.size())
                if (dist(myPos[i], oe) <= oe.attackRange) {
                    cntInRange++;
                    shots += (my[i].health + 4) / 5;
                }
            if (shots * cntInRange / 2 * 5 < oe.health) {
                res.main -= cntInRange * 100;
            }
            // if (verbose) {
            //     cerr << cntInRange << "," << shots << " vs " << oe.health << endl;
            // }
        }

    return res;
}

void optimizeMoves(const World& world, const vector<Entity>& my, const vector<Entity>& opp,
                   vector<int>& myMoves, const vector<int>& oppMoves, const vector<bool>& myCanMove,
                   bool nearBaseBattle) {
    Score score = getScore(my, opp, myMoves, oppMoves, world.finals && !nearBaseBattle);
    forn(i, my.size())
        if (myCanMove[i]) {
            forn(w, 5) {
                if (w == myMoves[i]) continue;
                const Cell nc = my[i].position ^ w;
                if (nc.inside() && !world.hasNonMovable(nc)) {
                    int was = myMoves[i];
                    myMoves[i] = w;
                    const Score cs = getScore(my, opp, myMoves, oppMoves, world.finals && !nearBaseBattle);
                    if (cs < score) { // operator< overloaded to select best
                        score = cs;
                    } else {
                        myMoves[i] = was;
                    }
                }
            }
        }
}

void optimizeMovesVec(const World& world, const vector<Entity>& my, const vector<Entity>& opp,
                      vector<int>& myMoves, const vector<vector<int>>& oppMovesVariants, const vector<bool>& myCanMove,
                      bool nearBaseBattle, Score& score) {
    forn(i, my.size())
        if (myCanMove[i]) {
            forn(w, 5) {
                if (w == myMoves[i]) continue;
                const Cell nc = my[i].position ^ w;
                if (nc.inside() && !world.hasNonMovable(nc)) {
                    int was = myMoves[i];
                    myMoves[i] = w;
                    Score cs(inf);
                    for (const auto& om : oppMovesVariants)
                        cs = max(cs, getScore(my, opp, myMoves, om, world.finals && !nearBaseBattle));
                    if (cs < score) { // operator< overloaded to select best
                        score = cs;
                    } else {
                        myMoves[i] = was;
                    }
                }
            }
        }
}

// #define BF

void bfBattle(const World& world, const GameStatus& st, const vector<Entity>& tobf) {
    vector<Entity> my, opp;
    for (const auto& e : tobf)
        if (e.playerId == world.myId) {
            my.push_back(e);
        } else {
            opp.push_back(e);
        }

    if (my.empty()) return;
    #ifdef BF
    cerr << "> bfBattle " << my.size() << " vs " << opp.size() << endl;
    #endif
    // unsigned tt = elapsed();

    bool nearBaseBattle = false;
    for (const Entity& e : opp)
        if (st.closeToBaseIds.count(e.id)) {
            nearBaseBattle = true;
            break;
        }

    sort(my.begin(), my.end(), [](const Entity& a, const Entity& b)
         { return a.health != b.health ? a.health < b.health : a.id < b.id; });
    sort(opp.begin(), opp.end(), [](const Entity& a, const Entity& b)
         { return a.health != b.health ? a.health < b.health : a.id < b.id; });

    ubf++;
    for (const auto& me : my)
        for (const auto& oe : opp) {
            const int cd = dist(me, oe);
            if (cd <= me.attackRange) isShooting[me.id] = ubf;
            if (cd <= oe.attackRange) isShooting[oe.id] = ubf;
        }

    vector<int> myMoves(my.size(), 4);
    vector<int> oppMoves(opp.size(), 4);
    forn(i, my.size()) { myHealth[i] = my[i].health; myPos[i] = my[i].position; }
    forn(i, opp.size()) { oppHealth[i] = opp[i].health; oppPos[i] = opp[i].position; }

    doAttack(my, opp, myMoves, oppMoves, myPos, oppPos, oppHealth, true);
    forn(i, my.size())
        if (myTarget[i] != -1) {
            attackTarget[my[i].id] = opp[myTarget[i]].id;
        }
    if (my.size() >= 24 || opp.size() >= 24) {
        forn(i, my.size())
            frontMoves[my[i].id] = 2;
        return;
    }

    doAttack(opp, my, oppMoves, myMoves, oppPos, myPos, myHealth, false);
    vector<Entity> nmy, nopp;
    forn(i, my.size())
        if (myHealth[i] > 0) {
            nmy.push_back(my[i]);
            nmy.back().health = myHealth[i];
        }
    forn(i, opp.size())
        if (oppHealth[i] > 0) {
            nopp.push_back(opp[i]);
            nopp.back().health = oppHealth[i];
        }
    my = std::move(nmy);
    opp = std::move(nopp);
    myMoves.resize(my.size());
    oppMoves.resize(opp.size());

    // cerr << "first attack " << (elapsed() - tt) << "ms";

    sort(my.begin(), my.end(), [](const Entity& a, const Entity& b)
         { return a.health != b.health ? a.health < b.health : a.id < b.id; });
    sort(opp.begin(), opp.end(), [](const Entity& a, const Entity& b)
         { return a.health != b.health ? a.health < b.health : a.id < b.id; });

    vector<bool> myCanMove(my.size(), false);
    vector<bool> oppCanMove(opp.size(), false);
    fillCanMove(my, opp, myCanMove);
    fillCanMove(opp, my, oppCanMove);

    #ifdef BF
    cerr << "After initial shooting:\n";
    cerr << "my:";
    forn(i, my.size()) {
        cerr << "  " << my[i].id << my[i].position << ", hp " << my[i].health << ", can move: " << myCanMove[i] << endl;
    }
    cerr << "opp:";
    forn(i, opp.size()) {
        cerr << "  " << opp[i].id << opp[i].position << ", hp " << opp[i].health << ", can move: " << oppCanMove[i] << endl;
    }
    #endif

    fillMovesAttack(world, my, opp, myMoves, myCanMove);
    fillMovesAttack(world, opp, my, oppMoves, oppCanMove);

    vector<vector<int>> oppMovesVariants;
    oppMovesVariants.push_back(oppMoves);
    vector<int> dirMoves(opp.size());
    forn(w, 5) {
        forn(e, opp.size()) {
            dirMoves[e] = w;
            const Cell nc = opp[e].position ^ w;
            if (!nc.inside() || world.hasNonMovable(nc) || !oppCanMove[e]) {
                dirMoves[e] = 4;
            }
        }
        bool ok = true;
        for (const auto& v : oppMovesVariants)
            if (v == dirMoves) {
                ok = false;
                break;
            }
        if (ok) {
            oppMovesVariants.push_back(dirMoves);
        }
    }

    // cerr << ", init OMV " << (elapsed() - tt) << "ms";

    unsigned ts = elapsed();
    forn(it, 4) {
        #ifdef BF
        cerr << ">> OPT my\n";
        #endif
        optimizeMoves(world, my, opp, myMoves, oppMoves, myCanMove, nearBaseBattle);
        #ifdef BF
        cerr << "after opt:"; forn(i, my.size()) cerr << " " << my[i].id << my[i].position << "->" << myMoves[i]; cerr << endl;
        cerr << ">> OPT opp\n";
        #endif
        optimizeMoves(world, opp, my, oppMoves, myMoves, oppCanMove, nearBaseBattle);
        #ifdef BF
        cerr << "after opt:"; forn(i, opp.size()) cerr << " " << opp[i].id << opp[i].position << "->" << oppMoves[i]; cerr << endl;
        #endif
        bool ok = true;
        for (const auto& v : oppMovesVariants)
            if (v == oppMoves) {
                ok = false;
                break;
            }
        if (ok) {
            oppMovesVariants.push_back(oppMoves);
        }
    }
    const int opt8time = elapsed() - ts;
    // cerr << ", main opt " << (elapsed() - tt) << "ms";

    #ifdef BF
    cerr << "opp moves variants count: " << oppMovesVariants.size() << endl;
    #endif

    const int iters = min(8, max(1, 77 * 8 / int(oppMovesVariants.size() * 7 * (opt8time + 1))));

    Score b1(inf);
    for (const auto& om : oppMovesVariants)
        b1 = max(b1, getScore(my, opp, myMoves, om, world.finals && !nearBaseBattle));
    forn(it, iters) {
        optimizeMovesVec(world, my, opp, myMoves, oppMovesVariants, myCanMove, nearBaseBattle, b1);
    }

    vector<int> myMovesBack(my.size(), 4);
    fillMovesAttack(world, my, opp, myMovesBack, myCanMove, -1);
    Score b2(inf);
    for (const auto& om : oppMovesVariants)
        b2 = max(b2, getScore(my, opp, myMovesBack, om, world.finals && !nearBaseBattle));
    forn(it, iters) {
        optimizeMovesVec(world, my, opp, myMovesBack, oppMovesVariants, myCanMove, nearBaseBattle, b2);
    }

    // cerr << ", two opt vec " << (elapsed() - tt) << "ms";

    #ifdef BF
    cerr << "after optimizations:\n";
    cerr << "my:\n";
    forn(i, my.size()) cerr << " " << my[i].id << my[i].position << "->" << myMoves[i]; cerr << " - score " << b1 << endl;
    forn(i, my.size()) cerr << " " << my[i].id << my[i].position << "->" << myMovesBack[i]; cerr << " - score " << b2 << endl;

    cerr << "=============== myMovesBack option =====================\n";
    for (const auto& om : oppMovesVariants) {
        cerr << "vs ";
        forn(i, opp.size()) cerr << " " << opp[i].id << opp[i].position << "->" << om[i];
        cerr << " - score " << getScore(my, opp, myMovesBack, om, world.finals, true) << endl;
    }
    cerr << "=============== myMoves option =====================\n";
    for (const auto& om : oppMovesVariants) {
        cerr << "vs ";
        forn(i, opp.size()) cerr << " " << opp[i].id << opp[i].position << "->" << om[i];
        cerr << " - score " << getScore(my, opp, myMoves, om, world.finals, true) << endl;
    }
    #endif

    if (b2 < b1) {
        myMoves = myMovesBack;
        b1 = b2;
    }

    forn(w, 5) {
        forn(e, my.size()) {
            myMovesBack[e] = w;
            const Cell nc = my[e].position ^ w;
            if (!nc.inside() || world.hasNonMovable(nc) || !myCanMove[e]) {
                myMovesBack[e] = 4;
            }
        }
        b2 = Score(inf);
        for (const auto& om : oppMovesVariants)
            b2 = max(b2, getScore(my, opp, myMoves, om, world.finals && !nearBaseBattle));
        forn(it, iters) {
            optimizeMovesVec(world, my, opp, myMovesBack, oppMovesVariants, myCanMove, nearBaseBattle, b2);
        }

        #ifdef BF
        cerr << "after opt moves to " << w << ":"
        forn(i, my.size()) cerr << " " << my[i].id << my[i].position << "->" << myMovesBack[i]; cerr << " - score " << b2 << endl;
        #endif
        if (b2 < b1) {
            cerr << "B" << w << "!";
            myMoves = myMovesBack;
            b1 = b2;
        }
    }

    // cerr << ", opt DM " << (elapsed() - tt) << "ms\n";

    forn(i, my.size())
        frontMoves[my[i].id] = myMoves[i];
    forn(i, opp.size())
        frontMoves[opp[i].id] = oppMoves[i];
}

void assignFrontMoves(const World& world, const GameStatus& st) {
    vector<Entity> warEntities;
    frontMoves.clear();
    attackTarget.clear();
    for (int p = 1; p <= 4; p++) {
        for (int wi : world.warriors[p])
            warEntities.push_back(world.entityMap.at(wi));
        for (int bi : world.buildings[p]) {
            const auto& b = world.entityMap.at(bi);
            if (b.entityType == EntityType::TURRET && b.active) {
                warEntities.push_back(b);
            }
        }
    }

    const int N = warEntities.size();
    vector<vector<int>> g(N);
    forn(i, N)
        forn(j, i) {
            const auto& w1 = warEntities[i];
            const auto& w2 = warEntities[j];
            if (dist(w1, w2) <= max(w1.attackRange, w2.attackRange) + 2 && w1.playerId != w2.playerId) {
                g[i].push_back(j);
                g[j].push_back(i);
            }
        }

    vector<bool> used(N);
    forn(i, N) {
        if (used[i] || g[i].empty()) continue;

        vector<int> q;
        size_t qb = 0;
        q.push_back(i);
        used[i] = true;

        while (qb < q.size()) {
            int x = q[qb++];
            for (int j : g[x]) {
                if (!used[j]) {
                    q.push_back(j);
                    used[j] = true;
                }
            }
        }

        vector<Entity> tobf;
        for (int x : q) {
            const auto& ent = warEntities[x];
            tobf.push_back(ent);
            possibleTargets[ent.id].clear();
            for (int j : g[x])
                possibleTargets[ent.id].push_back(warEntities[j].id);
        }
        bfBattle(world, st, tobf);
    }
}

struct TargetCand {
    int dist;
    int myId, oppId;
    TargetCand(int a, int b, int c): dist(a), myId(b), oppId(c) {}
};
bool operator<(const TargetCand& a, const TargetCand& b) {
    return a.dist < b.dist;
}

unordered_set<int> flankers;

void assignFinalsTargets(const World& world, const GameStatus& st) {
    vector<TargetCand> cand;
    needBuildArmy = false;
    frontTarget.clear();
    unordered_set<int> usedMy;
    unordered_map<int, int> usedOpp;

    vector<Entity> cside;
    for (const auto& w : world.myWarriors) {
        int cld = inf;
        for (const auto& oe : world.oppEntities) {
            if (usedOpp[oe.id] == 2) continue;
            if (w.position.y > 64 && oe.position.y < 64) continue;
            if (w.position.x > 64 && oe.position.x < 64) continue;
            const int cd = dist(w.position, oe);
            if (cd < cld) cld = cd;
            cand.emplace_back(cd, w.id, oe.id);
        }
        if (cld > 8) cside.emplace_back(w);
    }

    for (const auto& oe : st.enemiesCloseToWorkers) {
        forn(it, 2) {
            int cld = inf, ci = -1;
            for (const auto& w : cside) {
                if (usedMy.count(w.id)) continue;
                const int cd = dist(w.position, oe);
                if (cd < cld) {
                    cld = cd;
                    ci = w.id;
                }
            }
            if (ci != -1) {
                // cerr << "assign " << ci << " for " << oe.id << "@" << oe.position << endl;
                usedOpp[oe.id]++;
                usedMy.insert(ci);
                frontTarget[ci] = oe.position;
            }
        }
    }

    for (const auto& oi : st.closeToBaseIds) {
        const auto& oe = world.entityMap.at(oi);
        if (usedOpp[oe.id] == 2) continue;
        forn(it, 2) {
            int cld = inf, ci = -1;
            for (const auto& w : cside) {
                if (usedMy.count(w.id)) continue;
                const int cd = dist(w.position, oe);
                if (cd < cld) {
                    cld = cd;
                    ci = w.id;
                }
            }
            if (ci != -1) {
                // cerr << "assign " << ci << " for " << oe.id << "@" << oe.position << endl;
                usedOpp[oe.id]++;
                usedMy.insert(ci);
                frontTarget[ci] = oe.position;
            }
        }
    }

    sort(cand.begin(), cand.end());
    sort(cside.begin(), cside.end(),
         [](const Entity& a, const Entity& b)
         { return a.position.x - a.position.y < b.position.x - b.position.y; });
    unordered_set<int> uside;

    const int K = 3;
    forn(i, cside.size()) {
        if (i < K || i >= cside.size() - K)
            if (usedMy.find(cside[i].id) == usedMy.end())
                uside.insert(cside[i].id);
    }

    for (const auto& c : cand) {
        int myId = c.myId;
        if (uside.count(myId)) continue;
        int oppId = c.oppId;
        if (usedMy.find(myId) == usedMy.end()) {
            int& uo = usedOpp[oppId];
            if (uo < 2) {
                uo++;
                usedMy.insert(myId);
                frontTarget[myId] = world.entityMap.at(oppId).position;
            }
        }
    }

    vector<pair<Cell, int>> freeWarriors;
    for (const auto& w : world.myWarriors)
        if (usedMy.find(w.id) == usedMy.end()) {
            freeWarriors.emplace_back(w.position, w.id);
        }

    sort(freeWarriors.begin(), freeWarriors.end(),
         [](const pair<Cell, int>& a, const pair<Cell, int>& b)
         { return a.first.x - a.first.y < b.first.x - b.first.y; });

    const int H = freeWarriors.size() / 2;
    forn(i, H) {
        const auto& [pos, id] = freeWarriors[i];
        if ((pos.x >= 19 && pos.y >= 70) || flankers.count(id)) {
            frontTarget[id] = Cell(77, 77);
            flankers.insert(id);
        } else {
            frontTarget[id] = Cell(max(pos.x, 19), 77);
        }
    }
    for (size_t i = H; i < freeWarriors.size(); i++) {
        const auto& [pos, id] = freeWarriors[i];
        if ((pos.y >= 19 && pos.x >= 70) || flankers.count(id)) {
            frontTarget[id] = Cell(77, 77);
            flankers.insert(id);
        } else {
            frontTarget[id] = Cell(77, max(pos.y, 19));
        }
    }
}