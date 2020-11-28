#ifndef _MODEL_VEC2_INT_HPP_
#define _MODEL_VEC2_INT_HPP_

#include "../Stream.hpp"
#include <string>
#include <cmath>
#include <iostream>

class Vec2Int {
public:
    int x;
    int y;
    Vec2Int();
    Vec2Int(int x, int y);
    static Vec2Int readFrom(InputStream& stream);
    void writeTo(OutputStream& stream) const;
    bool operator ==(const Vec2Int& other) const;

    bool inside() const {
        return 0 <= x && x < 80 && 0 <= y && y < 80;
    }
};
namespace std {
    template<>
    struct hash<Vec2Int> {
        size_t operator ()(const Vec2Int& value) const;
    };
}

using Cell = Vec2Int;

int dist(const Cell& a, const Cell& b);
Cell operator+(const Cell& a, const Cell& b);
Cell operator-(const Cell& a, const Cell& b);
std::ostream& operator<<(std::ostream& out, const Vec2Int& v);

#endif
