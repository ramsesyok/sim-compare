#pragma once

#include <cstddef>
#include <string>
#include <unordered_map>
#include <vector>

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
 * @brief SoA(Structure of Arrays)形式でオブジェクトの属性を保持する入れ物です。
 *
 * @details 1つのオブジェクトを構造体でまとめるのではなく、属性ごとに配列を分けて管理します。
 *          これにより、例えば位置だけを連続メモリとして処理でき、CPUキャッシュ効率が上がります。
 *          逆にAoS(Array of Structures)は1個体の情報がまとまるため、個体単位の処理は書きやすいです。
 *          学習用として、どちらの利点も意識できるようにコメントを残しています。
 */
struct SoaStorage {
    std::vector<std::string> object_ids;
    std::vector<std::string> team_ids;
    std::vector<jsonobj::Role> roles;
    std::vector<int> start_secs;

    std::vector<double> ecef_xs;
    std::vector<double> ecef_ys;
    std::vector<double> ecef_zs;

    std::vector<RoutePoint> route_points;
    std::vector<size_t> route_offsets;
    std::vector<size_t> route_counts;

    std::vector<double> segment_end_secs;
    std::vector<size_t> segment_offsets;
    std::vector<size_t> segment_counts;

    std::vector<double> total_duration_secs;

    std::vector<std::unordered_map<std::string, DetectionInfo>> detect_states;
    std::vector<bool> has_detonated;
};
