#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "geo.hpp"
#include "jsonobj/scenario.hpp"
#include "route.hpp"

/**
 * @brief 探知した相手の情報を保持し、失探判定に利用するための構造体です。
 *
 * @details 斥候ごとに「前の時刻で探知していた相手」を覚えるために使います。
 *          これにより、今は見えていない相手を「失探」として記録できるようになります。
 */
struct DetectionInfo {
    double lat_deg = 0.0;
    double lon_deg = 0.0;
    double alt_m = 0.0;
    int distance_m = 0;
};

/**
 * @brief 1個体分の情報をまとめたAoS用の構造体です。
 *
 * @details AoS(Array of Structures)では、1個体の属性を1つの構造体にまとめます。
 *          これにより「個体単位の処理」が読みやすくなり、状態のまとまりを把握しやすくします。
 */
struct AosObject {
    std::string object_id;
    std::string team_id;
    jsonobj::Role role{};
    int start_sec = 0;

    Ecef position{0.0, 0.0, 0.0};
    std::vector<RoutePoint> route{};
    std::vector<double> segment_end_secs{};
    double total_duration_sec = 0.0;

    std::unordered_map<std::string, DetectionInfo> detect_state{};
    bool has_detonated = false;
};

/**
 * @brief AoS形式の配列をまとめたストレージです。
 *
 * @details 1個体ごとの構造体を配列で持ち、個体単位の更新処理をわかりやすくします。
 */
struct AosStorage {
    std::vector<AosObject> objects{};
};
