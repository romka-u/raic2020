#pragma once
#pragma GCC optimize("-O3")
#pragma GCC optimize("inline")
#pragma GCC optimize("omit-frame-pointer")
#pragma GCC optimize("unroll-loops")
#include <algorithm>
#include <vector>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <array>
#include <thread>
#include <set>
#include <cassert>
#include <tuple>
#include <cmath>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <memory.h>
#include <cstdlib>

using namespace std;

#define forn(i, n) for (int i = 0; i < (int)(n); i++)
typedef long long ll;
typedef unsigned long long ull;
typedef pair<int, int> pii;

const int dx[] = {-1, 0, 1, 0, 0};
const int dy[] = {0, -1, 0, 1, 0};

#define sqr(x) (x) * (x)
// #define contains(v, item) std::find(v.begin(), v.end(), item) != v.end()

const int inf = 1e9;

const bool TURRETS_CHEESE = true;

struct Score {
    int main;
    double aux;

    Score() {}
    Score(int m): main(m), aux(0) {}
    Score(int m, int a): main(m), aux(a) {}
    Score(int m, double a): main(m), aux(a) {}
};

bool operator<(const Score& a, const Score& b) {
    if (a.main != b.main) return a.main > b.main;
    return a.aux > b.aux;
}

ostream& operator<<(ostream& out, const Score& sc) {
    return out << "[" << sc.main << "|" << sc.aux << "]";
}