#pragma once

#include <utility>
#include <vector>

#include "geo.hpp"
#include "scenario.hpp"

/**
 * @brief 経路点の計算結果をまとめる構造体です。
 *
 * @details 緯度経度とECEF座標の両方を保持し、移動の詳細はどの役割でも共通なので分離します。
 */
struct RoutePoint {
    double lat_deg = 0.0;
    double lon_deg = 0.0;
    double alt_m = 0.0;
    double speeds_kph = 0.0;
    Ecef ecef;
};

/**
 * @brief シナリオの経路点を計算済みの経路点へ変換します。
 */
std::vector<RoutePoint> buildRoute(const std::vector<simoop::Waypoint> &route);
/**
 * @brief 経路の各区間に必要な時間と総移動時間を計算します。
 */
std::pair<std::vector<double>, double> buildSegmentTimes(const std::vector<RoutePoint> &route);
