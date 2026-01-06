#pragma once

#include <utility>
#include <vector>

#include "geo.hpp"
#include "scenario.hpp"

// 経路点の計算結果をまとめるため、緯度経度とECEF座標の両方を保持します。
struct RoutePoint {
    double lat_deg = 0.0;
    double lon_deg = 0.0;
    double alt_m = 0.0;
    double speeds_kph = 0.0;
    Ecef ecef;
};

std::vector<RoutePoint> buildRoute(const std::vector<simoop::Waypoint> &route);
std::pair<std::vector<double>, double> buildSegmentTimes(const std::vector<RoutePoint> &route);
