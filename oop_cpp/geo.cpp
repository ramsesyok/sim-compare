#include "geo.hpp"

#include <cmath>

double distanceEcef(const Ecef &a, const Ecef &b) {
    // ECEF空間でのユークリッド距離を計算するために、3軸の差分を合成します。
    // 役割によらず共通の計算なので、クラスではなく関数に切り出しています。
    double dx = a.x - b.x;
    double dy = a.y - b.y;
    double dz = a.z - b.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

Ecef geodeticToEcef(double lat_deg, double lon_deg, double alt_m) {
    // WGS84の測地座標をECEF座標に変換するため、回転楕円体のパラメータを使って計算します。
    // 変換ロジックは共通処理として独立させ、各クラスでの重複を避けます。
    constexpr double a = 6378137.0;
    constexpr double f = 1.0 / 298.257223563;
    constexpr double e2 = f * (2.0 - f);

    double lat = lat_deg * M_PI / 180.0;
    double lon = lon_deg * M_PI / 180.0;

    double sin_lat = std::sin(lat);
    double cos_lat = std::cos(lat);
    double sin_lon = std::sin(lon);
    double cos_lon = std::cos(lon);

    double n = a / std::sqrt(1.0 - e2 * sin_lat * sin_lat);

    return Ecef{
        (n + alt_m) * cos_lat * cos_lon,
        (n + alt_m) * cos_lat * sin_lon,
        (n * (1.0 - e2) + alt_m) * sin_lat,
    };
}

void ecefToGeodetic(const Ecef &pos, double &lat_deg, double &lon_deg, double &alt_m) {
    // ECEFから緯度経度高度へ戻すため、反復近似で緯度と高度を求めます。
    // 計算は純粋関数として扱い、状態は持たない設計にしています。
    constexpr double a = 6378137.0;
    constexpr double f = 1.0 / 298.257223563;
    constexpr double e2 = f * (2.0 - f);

    double p = std::sqrt(pos.x * pos.x + pos.y * pos.y);
    double lat = std::atan2(pos.z, p);
    double lon = std::atan2(pos.y, pos.x);
    double alt = 0.0;

    for (int i = 0; i < 5; ++i) {
        double sin_lat = std::sin(lat);
        double n = a / std::sqrt(1.0 - e2 * sin_lat * sin_lat);
        alt = p / std::cos(lat) - n;
        lat = std::atan2(pos.z, p * (1.0 - e2 * n / (n + alt)));
        lon = std::atan2(pos.y, pos.x);
    }

    lat_deg = lat * 180.0 / M_PI;
    lon_deg = lon * 180.0 / M_PI;
    alt_m = alt;
}
