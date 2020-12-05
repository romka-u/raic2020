#pragma once

#include "model/Entity.hpp"
#include "model/Vec2Float.hpp"

#ifdef DEBUG
#include "visualizer.h"
#endif
#include "common.h"
#include "world.h"

struct TickDrawInfo {
    int myId;
    vector<Entity> entities;
};

TickDrawInfo tickInfo[1010];
int currentDrawTick, maxDrawTick;
Vec2Float clickedPointWorld, clickedPointScreen;

#ifdef DEBUG
Visualizer v;
#endif
const int SZ = 100;

void draw() {
    #ifdef DEBUG
    const auto& info = tickInfo[currentDrawTick];

    v.p.setPen(darkWhitePen);
    v.p.setBrush(darkWhiteBrush);
    v.p.drawRect(-SZ/4, -SZ/4, SZ*80+SZ/2, SZ*80+SZ/2);
    v.p.setBrush(blackBrush);
    v.p.drawRect(0, 0, SZ*80, SZ*80);
    unordered_map<EntityType, int> cnt[4];

    Cell clickedCell(int(clickedPointWorld.x / SZ), int(clickedPointWorld.y / SZ));
    const Entity* clickedEntity = nullptr;
    
    for (const auto& e : info.entities) {
        const Cell& pos = e.position;
        if (pos == clickedCell) {
            clickedEntity = &e;
        }
        if (e.entityType == EntityType::RESOURCE) {
            v.p.setBrush(grayBrush);
            v.p.setPen(grayPen);
            const double sz = SZ * e.health / 30;
            const double gap = (SZ - sz) / 2;
            v.p.drawRect(pos.x * SZ + gap, pos.y * SZ + gap, sz, sz);
        } else {
            const int zid = *e.playerId - 1;
            cnt[zid][e.entityType]++;
            v.p.setBrush(brushesPerPlayer[zid]);
            v.p.setPen(pensPerPlayer[zid]);
            const int sz = props.at(e.entityType).size;
            const int maxhp = props.at(e.entityType).maxHealth;
            v.p.drawRect((pos.x + 0.05) * SZ, (pos.y + 0.05) * SZ, (sz - 0.1) * SZ, (sz - 0.1) * SZ);

            v.p.setBrush(blackBrush);
            v.p.setPen(blackPen);
            v.p.drawRect((pos.x + 0.15) * SZ, (pos.y + 0.85 + sz - 1) * SZ, (sz - 0.3) * SZ, 0.16 * SZ);
            v.p.setBrush(whiteBrush);
            v.p.setPen(whitePen);            
            v.p.drawRect((pos.x + 0.15) * SZ, (pos.y + 0.85 + sz - 1) * SZ, (sz - 0.3) * e.health / maxhp * SZ, 0.16 * SZ);

            v.p.setPen(darkWhitePen);
            v.p.setBrush(darkWhiteBrush);

            if (e.entityType == EntityType::RANGED_UNIT) {
                QPolygonF polygon;
                polygon << QPointF((pos.x + 0.5) * SZ, (pos.y + 0.7) * SZ);
                polygon << QPointF((pos.x + 0.1) * SZ, (pos.y + 0.1) * SZ);
                polygon << QPointF((pos.x + 0.9) * SZ, (pos.y + 0.1) * SZ);
                v.p.drawConvexPolygon(polygon);
            }
            if (e.entityType == EntityType::MELEE_UNIT) {
                v.p.drawRect((pos.x + 0.3) * SZ, (pos.y + 0.3) * SZ, SZ * 0.4, SZ * 0.4);
            }
        }
    }

    v.switchToWindow();

    v.p.setBrush(blackBrush);
    v.p.setPen(blackPen);
    v.p.drawRect(1250, 0, 500, 1111);

    QFont fs("Andale Mono", 12);
    fs.setBold(true);
    char buf[77];

    if (clickedEntity) {
        // v.p.setPen(balloonPen);
        // v.p.setBrush(balloonBrush);
        const int H = 70;
        const int W = 111;
        QPainterPath path;
        path.addRoundedRect(QRectF(clickedPointScreen.x, clickedPointScreen.y - H - 5, W, H), 10, 10);
        v.p.fillPath(path, balloonBrush);

        if (clickedEntity->playerId)
            v.p.setPen(pensPerPlayer[*clickedEntity->playerId - 1]);
        else
            v.p.setPen(blackPen);

        sprintf(buf, "Id: %d", clickedEntity->id);
        v.p.drawText(clickedPointScreen.x + 10, clickedPointScreen.y - H + 15, QString(buf));

        sprintf(buf, "Squad Id: %d", 2);
        v.p.drawText(clickedPointScreen.x + 10, clickedPointScreen.y - H + 30, QString(buf));
    }    

    QFont f1("Andale Mono", 20);
    f1.setBold(true);
    f1.setStyleStrategy(QFont::PreferAntialias);
    // QFontInfo in(f1);
    // QString family = in.family();
    // cerr << family.toStdString() << endl;

    v.p.setFont(f1);
    v.p.setPen(whitePen);
    sprintf(buf, "Tick: %d / %d", currentDrawTick, maxDrawTick);
    v.p.drawText(1379, 25, QString(buf));

    v.p.drawText(1300, 50, "Workers");
    v.p.drawText(1300, 75, "Melee");
    v.p.drawText(1300, 100, "Ranged");
    v.p.drawText(1300, 125, "Houses");
    v.p.drawText(1300, 150, "Turrets");

    forn(ip, 4) {
        v.p.setPen(pensPerPlayer[ip]);
        v.p.drawText(1400 + 50 * ip, 50, QString::number(cnt[ip][EntityType::BUILDER_UNIT]));
        v.p.drawText(1400 + 50 * ip, 75, QString::number(cnt[ip][EntityType::MELEE_UNIT]));
        v.p.drawText(1400 + 50 * ip, 100, QString::number(cnt[ip][EntityType::RANGED_UNIT]));
        v.p.drawText(1400 + 50 * ip, 125, QString::number(cnt[ip][EntityType::HOUSE]));
        v.p.drawText(1400 + 50 * ip, 150, QString::number(cnt[ip][EntityType::TURRET]));
    }

    #endif
}