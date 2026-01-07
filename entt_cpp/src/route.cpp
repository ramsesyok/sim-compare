/**
 * @file route.cpp
 * @brief ルート情報をECEFに展開し、区間時間を計算する実装ファイルです。
 *
 * @details ルートの前処理を関数に分離しておくことで、更新ループを簡潔にできます。
 */
#include "route.hpp"

#include <limits>

/**
 * @brief シナリオの経路点をECEFへ変換した配列を作成します。
 *
 * @details 変換結果は更新処理で再利用するため、ここで一度だけ計算します。
 */
std::vector<RoutePoint> buildRoute(const std::vector<jsonobj::Waypoint> &route) {
    std::vector<RoutePoint> result;
    result.reserve(route.size());
    for (const auto &wp : route) {
        // 測地座標からECEFに変換して、後続の位置補間を簡単にします。
        // 変換は共通処理として関数に集約し、各所で同じ式を持たないようにします。
        result.push_back(RoutePoint{
            wp.getLatDeg(),
            wp.getLonDeg(),
            wp.getAltM(),
            wp.getSpeedsKph(),
            geodeticToEcef(wp.getLatDeg(), wp.getLonDeg(), wp.getAltM()),
        });
    }
    return result;
}

/**
 * @brief ルート区間の終了時刻配列と合計所要時間を計算します。
 *
 * @details 1区間ごとの累積時間を先に計算し、位置補間で高速に参照できるようにします。
 */
std::pair<std::vector<double>, double> buildSegmentTimes(const std::vector<RoutePoint> &route) {
    if (route.size() < 2) {
        return {std::vector<double>{}, 0.0};
    }

    std::vector<double> segment_ends;
    segment_ends.reserve(route.size() - 1);
    double acc = 0.0;

    for (size_t i = 0; i + 1 < route.size(); ++i) {
        // 区間距離と速度から移動に必要な秒数を算出します。
        // 前計算しておくことで、更新ループ内の負荷を減らします。
        double distance = distanceEcef(route[i].ecef, route[i + 1].ecef);
        double speed_mps = (route[i].speeds_kph * 1000.0) / 3600.0;
        double duration = std::numeric_limits<double>::infinity();
        if (speed_mps > 0.0) {
            duration = distance / speed_mps;
        }
        acc += duration;
        segment_ends.push_back(acc);
    }

    return {segment_ends, acc};
}
