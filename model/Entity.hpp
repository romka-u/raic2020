#ifndef _MODEL_ENTITY_HPP_
#define _MODEL_ENTITY_HPP_

#include "../Stream.hpp"
#include "EntityType.hpp"
#include "Vec2Int.hpp"
#include <memory>
#include <stdexcept>
#include <string>

class Entity {
public:
    int id;
    int playerId;
    EntityType entityType;
    Vec2Int position;
    int health;
    bool active;
    int size;
    int attackRange;
    int maxHealth;
    Entity();
    Entity(int id, int playerId, EntityType entityType, Vec2Int position, int health, bool active);
    static Entity readFrom(InputStream& stream);
    void writeTo(OutputStream& stream) const;
};

#endif
