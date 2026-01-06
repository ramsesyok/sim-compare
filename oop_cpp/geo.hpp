#pragma once

#include <cstddef>

// ECEF座標を扱うための構造体で、位置計算の共通データとして使います。
// 役割ごとに計算が変わらないため、共通の小さな構造体として集約します。
struct Ecef {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

double distanceEcef(const Ecef &a, const Ecef &b);
Ecef geodeticToEcef(double lat_deg, double lon_deg, double alt_m);
void ecefToGeodetic(const Ecef &pos, double &lat_deg, double &lon_deg, double &alt_m);
