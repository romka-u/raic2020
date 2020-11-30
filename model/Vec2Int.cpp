#include "Vec2Int.hpp"

Vec2Int::Vec2Int() { }
Vec2Int::Vec2Int(int x, int y) : x(x), y(y) { }
Vec2Int Vec2Int::readFrom(InputStream& stream) {
    Vec2Int result;
    result.x = stream.readInt();
    result.y = stream.readInt();
    return result;
}
void Vec2Int::writeTo(OutputStream& stream) const {
    stream.write(x);
    stream.write(y);
}
bool Vec2Int::operator ==(const Vec2Int& other) const {
    return x == other.x && y == other.y;
}
bool Vec2Int::operator !=(const Vec2Int& other) const {
    return x != other.x || y != other.y;
}
size_t std::hash<Vec2Int>::operator ()(const Vec2Int& value) const {
    size_t result = 0;
    result ^= std::hash<int>{}(value.x) + 0x9e3779b9 + (result<<6) + (result>>2);
    result ^= std::hash<int>{}(value.y) + 0x9e3779b9 + (result<<6) + (result>>2);
    return result;
}

int dist(const Cell& a, const Cell& b) {
    return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

Cell operator+(const Cell& a, const Cell& b) {
    return Cell(a.x + b.x, a.y + b.y);
}

Cell operator-(const Cell& a, const Cell& b) {
    return Cell(a.x - b.x, a.y - b.y);
}

bool operator<(const Cell& a, const Cell& b) {
    if (a.x != b.x) return a.x < b.x;
    return a.y < b.y;
}

std::ostream& operator<<(std::ostream& out, const Vec2Int& v) {
    return out << "(" << v.x << "," << v.y << ")";
}