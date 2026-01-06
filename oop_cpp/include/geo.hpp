#pragma once

#include <cstddef>

/**
 * @brief ECEF座標を扱うための構造体です。
 *
 * @details 役割ごとに計算が変わらないため、共通の小さな構造体として集約します。
 */
struct Ecef {
    double x = 0.0;
    double y = 0.0;
    double z = 0.0;
};

/**
 * @brief ECEF空間での距離を計算します。
 */
double distanceEcef(const Ecef &a, const Ecef &b);
/**
 * @brief 測地座標(緯度経度高度)をECEF座標に変換します。
 */
Ecef geodeticToEcef(double lat_deg, double lon_deg, double alt_m);
/**
 * @brief ECEF座標を測地座標(緯度経度高度)に変換します。
 */
void ecefToGeodetic(const Ecef &pos, double &lat_deg, double &lon_deg, double &alt_m);
