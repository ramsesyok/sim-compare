#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "movable_object.hpp"
#include "spatial_hash.hpp"

// 探知した相手の情報を保持し、失探判定に利用するための構造体です。
// 状態をクラスの内側に閉じ込めることで、外部から不用意に変更されないようにします。
struct DetectionInfo {
    double lat_deg = 0.0;
    double lon_deg = 0.0;
    double alt_m = 0.0;
    int distance_m = 0;
};

// 斥候役の移動と探知を担当するクラスです。
// MovableObjectを継承し、探知の責務だけを追加することで、責務の分離を明確にします。
class ScoutObject : public MovableObject {
public:
    ScoutObject(std::string id,
                std::string team_id,
                int start_sec,
                std::vector<RoutePoint> route,
                std::vector<std::string> network,
                std::vector<double> segment_end_secs,
                double total_duration_sec,
                int detect_range_m,
                int comm_range_m);

    void updateDetection(int time_sec,
                         const std::unordered_map<CellKey, std::vector<int>, CellKeyHash> &spatial_hash,
                         const std::vector<SimObject *> &objects,
                         int self_index);

private:
    int m_detect_range_m = 0;
    int m_comm_range_m = 0;
    std::unordered_map<std::string, DetectionInfo> m_detect_state;
};
