#include "spatial_hash.hpp"

#include <cmath>

#include "ecs_components.hpp"

size_t CellKeyHash::operator()(const CellKey &key) const {
    // 3軸を混ぜ合わせることで、セル座標の分布が偏っても衝突を減らします。
    // 空間ハッシュの詳細はここに閉じ込め、他の箇所からは使いやすい関数だけを見せます。
    size_t h1 = std::hash<int>{}(key.x);
    size_t h2 = std::hash<int>{}(key.y);
    size_t h3 = std::hash<int>{}(key.z);
    return (h1 * 73856093u) ^ (h2 * 19349663u) ^ (h3 * 83492791u);
}

CellKey cellKey(const Ecef &pos, double cell_size) {
    return CellKey{
        static_cast<int>(std::floor(pos.x / cell_size)),
        static_cast<int>(std::floor(pos.y / cell_size)),
        static_cast<int>(std::floor(pos.z / cell_size)),
    };
}

std::unordered_map<CellKey, std::vector<entt::entity>, CellKeyHash> buildSpatialHash(
    const entt::registry &registry,
    const std::vector<entt::entity> &entities,
    double cell_size) {
    std::unordered_map<CellKey, std::vector<entt::entity>, CellKeyHash> result;
    if (cell_size <= 0.0) {
        return result;
    }
    // 各オブジェクトをセルに割り当て、近傍探索の候補集合を高速に作ります。
    // 探知は斥候の責務ですが、空間分割はここで共通処理として提供します。
    for (entt::entity entity : entities) {
        const auto &pos = registry.get<PositionComponent>(entity);
        CellKey key = cellKey(pos.ecef, cell_size);
        result[key].push_back(entity);
    }
    return result;
}
