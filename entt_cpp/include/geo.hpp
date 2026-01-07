/**
 * @file geo.hpp
 * @brief 測地座標とECEF座標の変換に関する宣言をまとめたヘッダです。
 *
 * @details 位置計算の共通処理を分離し、コードの重複を減らします。
 */
#pragma once

/**
 * @brief 地球中心座標系(ECEF)の座標をまとめる構造体です。
 *
 * @details 緯度経度高度から一度ECEFへ変換し、直線補間を簡単にします。
 */
struct Ecef {
    double x;
    double y;
    double z;
};

/**
 * @brief ECEF空間での距離を計算します。
 */
double distanceEcef(const Ecef &a, const Ecef &b);

/**
 * @brief 緯度経度高度をECEFへ変換します。
 */
Ecef geodeticToEcef(double lat_deg, double lon_deg, double alt_m);

/**
 * @brief ECEF座標を緯度経度高度へ変換します。
 */
void ecefToGeodetic(const Ecef &pos, double &lat_deg, double &lon_deg, double &alt_m);
