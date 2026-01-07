/**
 * @file route.hpp
 * @brief ルート情報の変換と区間時間計算の宣言をまとめたヘッダです。
 *
 * @details 経路点をECEFに変換し、更新処理で使いやすい形に整えます。
 */
#pragma once

#include <utility>
#include <vector>

#include "geo.hpp"
#include "jsonobj/scenario.hpp"

/**
 * @brief 経路上の1点をECEF座標で保持する構造体です。
 */
struct RoutePoint {
    double lat_deg;
    double lon_deg;
    double alt_m;
    double speeds_kph;
    Ecef ecef;
};

/**
 * @brief シナリオの経路点をECEFへ変換して保持します。
 */
std::vector<RoutePoint> buildRoute(const std::vector<jsonobj::Waypoint> &route);

/**
 * @brief 経路区間ごとの終了時間と総移動時間を算出します。
 */
std::pair<std::vector<double>, double> buildSegmentTimes(const std::vector<RoutePoint> &route);
