#pragma once

#include <unordered_map>
#include <vector>

#include "geo.hpp"

class SimObject;

// 空間ハッシュのキーとなる整数セル座標をまとめた構造体です。
struct CellKey {
    int x = 0;
    int y = 0;
    int z = 0;

    bool operator==(const CellKey &other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

// CellKeyをunordered_mapのキーとして扱うためのハッシュ関数です。
struct CellKeyHash {
    size_t operator()(const CellKey &key) const;
};

CellKey cellKey(const Ecef &pos, double cell_size);
std::unordered_map<CellKey, std::vector<int>, CellKeyHash> buildSpatialHash(
    const std::vector<SimObject *> &objects,
    double cell_size);
