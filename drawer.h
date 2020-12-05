#pragma once

#include "model/Entity.hpp"

#ifdef DEBUG
#include "visualizer.h"
#endif
#include "common.h"

struct TickDrawInfo {
    int myId;
    vector<Entity> entities;
};

TickDrawInfo tickInfo[1010];
int currentDrawTick, maxDrawTick;

#ifdef DEBUG
Visualizer v;
#endif
const int SZ = 12;

void draw() {
    #ifdef DEBUG
    const auto& info = tickInfo[currentDrawTick];
    
    for (const auto& e : info.entities) {
        if (e.entityType == EntityType::RESOURCE) {
            v.p.setBrush(blueBrush);
            const double sz = SZ * e.health / 30;
            const double gap = (SZ - sz) / 2;
            v.p.drawRect(e.position.x * SZ + gap, e.position.y * SZ + gap, sz, sz);
        }
    }

    v.switchToWindow();

    QFont f1("Andale Mono", 20);
    // QFontInfo in(f1);
    // QString family = in.family();
    // cerr << family.toStdString() << endl;

    v.p.setFont(f1);
    v.p.setPen(whitePen);
    char buf[77];
    sprintf(buf, "Tick: %d / %d", currentDrawTick, maxDrawTick);
    v.p.drawText(19, 19, QString(buf));
    #endif
}