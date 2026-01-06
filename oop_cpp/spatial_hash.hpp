#pragma once

#include <unordered_map>
#include <vector>

#include "geo.hpp"

class SimObject;

/**
 * @brief 空間ハッシュのキーとなる整数セル座標をまとめた構造体です。
 *
 * @details 探知判定を高速化するために、空間をセルで分割する設計です。
 */
struct CellKey {
    int x = 0;
    int y = 0;
    int z = 0;

    bool operator==(const CellKey &other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

/**
 * @brief CellKeyをunordered_mapのキーとして扱うためのハッシュ関数です。
 *
 * @details ハッシュ化の責務を分けておくと、空間分割の変更が容易になります。
 */
struct CellKeyHash {
    size_t operator()(const CellKey &key) const;
};

/**
 * @brief 座標からセルキーを計算します。
 */
CellKey cellKey(const Ecef &pos, double cell_size);
/**
 * @brief 空間ハッシュを構築して近傍探索の候補をまとめます。
 */
std::unordered_map<CellKey, std::vector<int>, CellKeyHash> buildSpatialHash(
    const std::vector<SimObject *> &objects,
    double cell_size);
