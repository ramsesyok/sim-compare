#include "spatial_hash.hpp"

#include <cmath>

#include "sim_object.hpp"

size_t CellKeyHash::operator()(const CellKey &key) const {
    // 3軸を混ぜ合わせることで、セル座標の分布が偏っても衝突を減らします。
    // 空間ハッシュの実装詳細はここに閉じ込め、他のクラスから隠蔽します。
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

std::unordered_map<CellKey, std::vector<int>, CellKeyHash> buildSpatialHash(
    const std::vector<SimObject *> &objects,
    double cell_size) {
    std::unordered_map<CellKey, std::vector<int>, CellKeyHash> result;
    if (cell_size <= 0.0) {
        return result;
    }
    // 各オブジェクトをセルに割り当て、近傍探索の候補集合を高速に作ります。
    // 探知の責務は斥候にありますが、空間分割は独立した関数として分けます。
    for (size_t i = 0; i < objects.size(); ++i) {
        CellKey key = cellKey(objects[i]->position(), cell_size);
        result[key].push_back(static_cast<int>(i));
    }
    return result;
}
