#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "movable_object.hpp"
#include "spatial_hash.hpp"

class EventLogger;

/**
 * @brief 探知した相手の情報を保持し、失探判定に利用するための構造体です。
 *
 * @details 状態をクラスの内側に閉じ込めることで、
 *          外部から不用意に変更されないようにします。
 */
struct DetectionInfo {
    double lat_deg = 0.0;
    double lon_deg = 0.0;
    double alt_m = 0.0;
    int distance_m = 0;
};

/**
 * @brief 斥候役の移動と探知を担当するクラスです。
 *
 * @details MovableObjectを継承し、探知の責務だけを追加することで責務の分離を明確にします。
 */
class ScoutObject : public MovableObject {
public:
    /**
     * @brief 斥候の性能パラメータを含めて初期化します。
     */
    ScoutObject(std::string id,
                std::string team_id,
                int start_sec,
                std::vector<RoutePoint> route,
                std::vector<std::string> network,
                std::vector<double> segment_end_secs,
                double total_duration_sec,
                int detect_range_m,
                int comm_range_m,
                EventLogger *event_logger);

    /**
     * @brief 近傍探索結果から探知・失探イベントを生成して出力します。
     */
    void updateDetection(int time_sec,
                         const std::unordered_map<CellKey, std::vector<int>, CellKeyHash> &spatial_hash,
                         const std::vector<SimObject *> &objects,
                         int self_index);

private:
    int m_detect_range_m = 0;
    int m_comm_range_m = 0;
    std::unordered_map<std::string, DetectionInfo> m_detect_state;
    EventLogger *m_event_logger = nullptr;
};
