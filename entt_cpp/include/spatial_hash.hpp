#pragma once

#include <unordered_map>
#include <vector>

#include "entt/entt.hpp"
#include "geo.hpp"

/**
 * @brief 空間ハッシュ用のセル座標を表す構造体です。
 *
 * @details 探知範囲ごとに空間を立方体セルに区切り、同じセル内の候補だけを探すために使います。
 */
struct CellKey {
    int x;
    int y;
    int z;
};

/**
 * @brief CellKey同士の等価判定を定義します。
 *
 * @details unordered_mapでキー比較が必要になるため、3軸が同じかを明示的に比較します。
 */
inline bool operator==(const CellKey &a, const CellKey &b) {
    return a.x == b.x && a.y == b.y && a.z == b.z;
}

/**
 * @brief CellKeyをunordered_mapで使えるようにするハッシュ関数です。
 */
struct CellKeyHash {
    size_t operator()(const CellKey &key) const;
};

/**
 * @brief 位置とセルサイズからセル座標を計算します。
 */
CellKey cellKey(const Ecef &pos, double cell_size);

/**
 * @brief ECSの位置コンポーネントから空間ハッシュを構築します。
 *
 * @details 探知処理の前に候補を絞り込むための前処理です。
 */
std::unordered_map<CellKey, std::vector<entt::entity>, CellKeyHash> buildSpatialHash(
    const entt::registry &registry,
    const std::vector<entt::entity> &entities,
    double cell_size);
