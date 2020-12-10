#pragma once

#include "model/Entity.hpp"
#include "model/Vec2Float.hpp"
#include "model/Player.hpp"

#ifdef DEBUG
#include "visualizer.h"
#endif
#include "common.h"
#include "world.h"
#include "game_status.h"

using Targets = vector<pair<Cell, Cell>>;

struct TickDrawInfo {
    int myId;
    vector<Entity> entities;
    vector<Player> players;
    Targets targets;
    GameStatus status;
    unordered_map<int, ostringstream> msg;
    unordered_map<int, int> squadId;
    vector<int> myPower;
};
#ifdef DEBUG
unordered_map<EntityType, string> ename = {
    {EntityType::WALL, "WALL"},
    {EntityType::HOUSE, "HOUSE"},
    {EntityType::BUILDER_BASE, "BUILDER_BASE"},
    {EntityType::BUILDER_UNIT, "BUILDER"},
    {EntityType::MELEE_BASE, "MELEE_BASE"},
    {EntityType::MELEE_UNIT, "MELEE"},
    {EntityType::RANGED_BASE, "RANGED_BASE"},
    {EntityType::RANGED_UNIT, "RANGED"},
    {EntityType::RESOURCE, "RESOURCE"},
    {EntityType::TURRET, "TURRET"}
};

QFont f85("Andale Mono", 85);
QFont f20("Andale Mono", 20);
QFont f12("Andale Mono", 12);
QFont f8("Andale Mono", 8);

Visualizer v;
#endif

const int SZ = 100;
TickDrawInfo tickInfo[1010];
int currentDrawTick, maxDrawTick;
Vec2Float clickedPointWorld, clickedPointScreen;
bool drawMyField, drawOppField, drawTargets, drawInfMap, colorBySquads, drawBorderGroups;


#ifdef DEBUG
void drawTriangle(double x1, double y1, double x2, double y2, double x3, double y3) {
    QPolygonF polygon;
    polygon << QPointF(x1, y1);
    polygon << QPointF(x2, y2);
    polygon << QPointF(x3, y3);
    v.p.drawConvexPolygon(polygon);
}

void drawBars(unordered_map<EntityType, int> cnt[4], int yOffset, const string& label, EntityType et, vector<int> plus={}, vector<int> minus={}) {
    v.p.setPen(whitePen);
    v.p.setFont(f20);
    v.p.drawText(1150, yOffset + 10, QString(label.c_str()));

    int mx = 1;
    forn(p, 4) mx = max(mx, cnt[p][et]);
    const int L = 1292;
    const int MAXW = 1650 - L;
    forn(p, 4) {
        v.p.setPen(pensPerPlayer[p]);
        v.p.setBrush(brushesPerPlayer[p]);
        const int top = yOffset + 22 * (p - 2);
        v.p.drawRect(L, top, MAXW * cnt[p][et] / mx, 21);

        v.p.setPen(whitePen);
        v.p.setFont(f20);
        v.p.drawText(L + 5, top + 18, QString::number(cnt[p][et]));

        v.p.setFont(f12);
        if (plus.size() > p && plus[p] > 0) {
            v.p.setPen(greenPen);
            v.p.setBrush(greenBrush);
            drawTriangle(L - 5, top + 9, L - 13, top + 9, L - 9, top + 2);
            v.p.setPen(whitePen);
            int offX = 30;
            if (plus[p] > 9) offX = 37;
            v.p.drawText(L - offX, top + 10, "+" + QString::number(plus[p]));
        }
        if (minus.size() > p && minus[p] > 0) {
            v.p.setPen(redPen);
            v.p.setBrush(redBrush);
            drawTriangle(L - 5, top + 13, L - 13, top + 13, L - 9, top + 20);
            v.p.setPen(whitePen);
            int offX = 30;
            if (minus[p] > 9) offX = 37;
            v.p.drawText(L - offX, top + 21, "-" + QString::number(minus[p]));
        }
    }
}

void drawDoubleBars(unordered_map<EntityType, int> cnt[4], int yOffset, const string& label, EntityType et1, EntityType et2) {
    v.p.setFont(f20);
    v.p.setPen(whitePen);
    v.p.drawText(1150, yOffset + 10, QString(label.c_str()));

    int mx = 1;
    forn(p, 4) mx = max(mx, cnt[p][et2]);
    const int L = 1292;
    const int MAXW = 1650 - L;
    forn(p, 4) {
        v.p.setPen(pensPerPlayer[p]);
        v.p.setBrush(brushesPerPlayerAlpha[p]);
        v.p.drawRect(L, yOffset + 22 * (p - 2), MAXW * cnt[p][et2] / mx, 21);
        v.p.setBrush(brushesPerPlayer[p]);
        v.p.drawRect(L, yOffset + 22 * (p - 2), MAXW * cnt[p][et1] / mx, 21);

        v.p.setPen(whitePen);
        v.p.drawText(L + 5, yOffset + 22 * (p - 1) - 4, QString::number(cnt[p][et1]) + "/" + QString::number(cnt[p][et2]));
    }
}

void drawUnderAttack(const TickDrawInfo& info, int eMap[88][88], const QColor& color, const std::function<bool(int)>& goodId) {
    v.p.setBrush(QBrush(color));
    v.p.setPen(QPen(color));

    for (const auto& e : info.entities) {
        if (!goodId(e.playerId)) continue;
        if (e.attackRange == 0) continue;

        for (int x = e.position.x - e.attackRange; x <= e.position.x + e.size - 1 + e.attackRange; x++)
            for (int y = e.position.y - e.attackRange; y <= e.position.y + e.size - 1 + e.attackRange; y++) {
                Cell nc(x, y);
                if (nc.inside() && dist(nc, e) <= e.attackRange && eMap[nc.x][nc.y] >= 0) {
                    v.p.drawRect(x * SZ, y * SZ, SZ, SZ);
                }
            }
    }
}

void drawTargetsLines(const Targets& targets) {
    v.p.setPen(QPen(QColor(222, 222, 222, 222), 8));
    for (const auto& p : targets) {
        v.p.drawLine((p.first.x + 0.5) * SZ, (p.first.y + 0.5) * SZ,
                     (p.second.x + 0.5) * SZ, (p.second.y + 0.5) * SZ);
    }
}

void drawBarsWithDeltas(
        unordered_map<EntityType, int> cnt[4],
        unordered_map<EntityType, int> cntPrev[4],
        unordered_map<EntityType, int> cntNew[4],
        int yOffset, const string& label, EntityType et) {
    vector<int> cp(4), cm(4);
    forn(zid, 4) {
        cp[zid] = cntNew[zid][et];
        cm[zid] = cntPrev[zid][et] - (cnt[zid][et] - cntNew[zid][et]);
    }
    drawBars(cnt, yOffset, label, et, cp, cm);
}

void drawInfMapOverlay(const vector<Entity>& entities, int eMap[88][88]) {
    pii infMap[88][88];
    calcInfMap(entities, eMap, infMap);
    v.p.setPen(QPen(Qt::transparent));
    forn(x, 80)
        forn(y, 80) {
            if (infMap[x][y].first != 0) {
                v.p.setBrush(brushesPerPlayerAlpha[infMap[x][y].first - 1]);
                v.p.drawRect(x * SZ, y * SZ, SZ, SZ);
            }
        }
}

void drawBorderGroupsInfo(const vector<Entity>& entities, const GameStatus& st) {
    v.p.setFont(f85);
    v.p.setPen(QPen(QColor(222, 222, 222), 10));
    char buf[13];
    buf[1] = 0;
    forn(x, 80) forn(y, 80)
        if (st.ubg[x][y] == st.ubit) {
            buf[0] = 'A' + st.borderGroup[x][y];
            v.p.drawText(x * SZ + 10, y * SZ + SZ - 10, QString(buf));
        }

    for (const auto& e : entities)
        if (st.unitsToCell.count(e.id)) {
            const Cell& from = e.position;
            const Cell& to = st.unitsToCell.at(e.id);
            v.p.drawLine((from.x + 0.5) * SZ, (from.y + 0.5) * SZ, (to.x + 0.5) * SZ, (to.y + 0.5) * SZ);
        }

    v.p.setPen(QPen(QColor(222, 192, 32), 16));
    v.p.setBrush(Qt::transparent);
    for (const auto& c : st.hotPoints) {
        v.p.drawEllipse((c.x + 0.5) * SZ - SZ, (c.y + 0.5) * SZ - SZ, 2 * SZ, 2 * SZ);
    }
}

void draw() {
    const auto& info = tickInfo[currentDrawTick];

    v.p.setPen(darkWhitePen);
    v.p.setBrush(darkWhiteBrush);
    v.p.drawRect(-SZ/4, -SZ/4, SZ*80+SZ/2, SZ*80+SZ/2);
    v.p.setBrush(blackBrush);
    v.p.drawRect(0, 0, SZ*80, SZ*80);
    unordered_map<EntityType, int> cnt[4], cntNew[4], cntPrev[4];
    unordered_set<int> prevIds;
    vector<int> income(4), expenses(4);
    if (currentDrawTick > 0) {
        for (const auto& e : tickInfo[currentDrawTick - 1].entities)
            if (e.entityType != EntityType::RESOURCE) {
                prevIds.insert(e.id);
                cntPrev[e.playerId - 1][e.entityType]++;
            }
    }

    Cell clickedCell(int(clickedPointWorld.x / SZ), int(clickedPointWorld.y / SZ));
    const Entity* clickedEntity = nullptr;

    int eMap[88][88];
    memset(eMap, 0, sizeof(eMap));
    
    for (const auto& e : info.entities) {
        const int valueToFill = props.at(e.entityType).canMove ? e.id : -e.id;
        forn(dx, e.size) forn(dy, e.size) eMap[e.position.x + dx][e.position.y + dy] = valueToFill;

        const Cell& pos = e.position;
        if (pos.x <= clickedCell.x && clickedCell.x < pos.x + e.size
            && pos.y <= clickedCell.y && clickedCell.y < pos.y + e.size) {
            clickedEntity = &e;
        }

        if (e.entityType == EntityType::RESOURCE) {
            v.p.setBrush(grayBrush);
            v.p.setPen(grayPen);
            const double sz = SZ * e.health / e.maxHealth;
            const double gap = (SZ - sz) / 2;
            v.p.drawRect(pos.x * SZ + gap, pos.y * SZ + gap, sz, sz);
        } else {
            const int zid = e.playerId - 1;
            cnt[zid][e.entityType]++;
            if (prevIds.find(e.id) == prevIds.end()) {
                expenses[zid] += props.at(e.entityType).cost;
                if (props.at(e.entityType).canMove)
                    expenses[zid] += cntPrev[zid][e.entityType] + cntNew[zid][e.entityType];
                cntNew[zid][e.entityType]++;
            }
            if (e.entityType == EntityType::HOUSE
                || e.entityType == EntityType::MELEE_BASE
                || e.entityType == EntityType::RANGED_BASE
                || e.entityType == EntityType::BUILDER_BASE)
                {
                    if (e.active)
                        cnt[zid][EntityType::FOOD_LIMIT] += props.at(e.entityType).populationProvide;
                }                
            else {
                cnt[zid][EntityType::FOOD_USAGE] += props.at(e.entityType).populationUse;
            }

            if (e.active) {
                v.p.setBrush(brushesPerPlayer[zid]);
                v.p.setPen(pensPerPlayer[zid]);
            } else {
                v.p.setBrush(paleBrush);
                v.p.setPen(palePen);
            }

            if (colorBySquads) {
                if (info.squadId.count(e.id)) {
                    v.p.setBrush(brushesPerPlayer[info.squadId.at(e.id)]);
                    v.p.setPen(pensPerPlayer[info.squadId.at(e.id)]);
                } else {
                    v.p.setBrush(paleBrush);
                    v.p.setPen(palePen);
                }
            }

            v.p.drawRect((pos.x + 0.05) * SZ, (pos.y + 0.05) * SZ, (e.size - 0.1) * SZ, (e.size - 0.1) * SZ);

            v.p.setBrush(blackBrush);
            v.p.setPen(blackPen);
            v.p.drawRect((pos.x + 0.15) * SZ, (pos.y + 0.85 + e.size - 1) * SZ, (e.size - 0.3) * SZ, 0.16 * SZ);
            v.p.setBrush(whiteBrush);
            v.p.setPen(whitePen);            
            v.p.drawRect((pos.x + 0.15) * SZ, (pos.y + 0.85 + e.size - 1) * SZ, (e.size - 0.3) * e.health / e.maxHealth * SZ, 0.16 * SZ);

            v.p.setPen(darkWhitePen);
            v.p.setBrush(darkWhiteBrush);

            if (e.entityType == EntityType::RANGED_UNIT) {
                drawTriangle((pos.x + 0.5) * SZ, (pos.y + 0.7) * SZ,
                             (pos.x + 0.1) * SZ, (pos.y + 0.1) * SZ,
                             (pos.x + 0.9) * SZ, (pos.y + 0.1) * SZ);
            }
            if (e.entityType == EntityType::MELEE_UNIT) {
                v.p.drawRect((pos.x + 0.3) * SZ, (pos.y + 0.3) * SZ, SZ * 0.4, SZ * 0.4);
            }
        }
    }

    if (drawOppField) drawUnderAttack(info, eMap, QColor(255, 0, 0, 64), [&info](int id) { return id != info.myId; });
    if (drawMyField) drawUnderAttack(info, eMap, QColor(0, 255, 0, 64), [&info](int id) { return id == info.myId; });
    if (drawTargets) drawTargetsLines(info.targets);
    if (drawInfMap) drawInfMapOverlay(info.entities, eMap);
    if (drawBorderGroups) drawBorderGroupsInfo(info.entities, info.status);

    if (currentDrawTick > 0) {
        forn(p, info.players.size()) {
            income[p] = info.players[p].resource - tickInfo[currentDrawTick - 1].players[p].resource + expenses[p];
        }
    }

    for (const auto& p : info.players) {
        cnt[p.id - 1][EntityType::SCORE] = p.score;
        cnt[p.id - 1][EntityType::RESOURCE] = p.resource;
    }

    v.switchToWindow();

    v.p.setBrush(blackBrush);
    v.p.setPen(blackPen);
    v.p.drawRect(1133, 0, 777, 1111);

    char buf[77];

    if (clickedEntity) {
        // v.p.setPen(balloonPen);
        // v.p.setBrush(balloonBrush);
        const int H = 52;
        sprintf(buf, "%s Id: %d", ename[clickedEntity->entityType].c_str(), clickedEntity->id);
        int W = strlen(buf) * 7 + 24;
        if (info.msg.count(clickedEntity->id)) {
            W = max(W, 24 + int(info.msg.at(clickedEntity->id).str().size()) * 7);
        }
        QPainterPath path;
        path.addRoundedRect(QRectF(clickedPointScreen.x, clickedPointScreen.y - H - 5, W, H), 10, 10);
        v.p.fillPath(path, balloonBrush);

        v.p.setFont(f12);
        v.p.setPen(blackPen);
        sprintf(buf, "Cell (%d, %d)", clickedCell.x, clickedCell.y);
        v.p.drawText(clickedPointScreen.x + 10, clickedPointScreen.y - H + 12, QString(buf));

        if (clickedEntity->playerId != -1)
            v.p.setPen(pensPerPlayer[clickedEntity->playerId - 1]);
        
        sprintf(buf, "%s Id: %d", ename[clickedEntity->entityType].c_str(), clickedEntity->id);
        v.p.drawText(clickedPointScreen.x + 10, clickedPointScreen.y - H + 24, QString(buf));

        if (info.msg.count(clickedEntity->id))
            v.p.drawText(clickedPointScreen.x + 10, clickedPointScreen.y - H + 36,
                         QString(info.msg.at(clickedEntity->id).str().c_str()));
    }    

    int yOffset = 100;
    drawBars(cnt, yOffset, "Score", EntityType::SCORE);
    yOffset += 100;
    if (currentDrawTick > 0) {
        drawBars(cnt, yOffset, "Resource", EntityType::RESOURCE, income, expenses);
    } else {
        drawBars(cnt, yOffset, "Resource", EntityType::RESOURCE);
    }
    yOffset += 100;
    drawDoubleBars(cnt, yOffset, "Food", EntityType::FOOD_USAGE, EntityType::FOOD_LIMIT); yOffset += 100;
    drawBarsWithDeltas(cnt, cntPrev, cntNew, yOffset, "Workers", EntityType::BUILDER_UNIT); yOffset += 100;
    drawBarsWithDeltas(cnt, cntPrev, cntNew, yOffset, "Range", EntityType::RANGED_UNIT); yOffset += 100;
    drawBarsWithDeltas(cnt, cntPrev, cntNew, yOffset, "Melee", EntityType::MELEE_UNIT); yOffset += 100;
    drawBarsWithDeltas(cnt, cntPrev, cntNew, yOffset, "Turrets", EntityType::TURRET); yOffset += 100;

    v.p.setFont(f20);
    v.p.setPen(whitePen);
    sprintf(buf, "Tick: %d / %d", currentDrawTick, maxDrawTick);
    v.p.drawText(1379, 25, QString(buf));

    v.p.setFont(f12);
    forn(i, info.status.attackersPower.size()) {
        sprintf(buf, "Front %c: %d vs %d", 'A'+i, info.status.attackersPower[i], info.myPower[i]);
        v.p.drawText(1200, yOffset + i * 16, buf);
    }
}
#endif