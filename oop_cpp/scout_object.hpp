#pragma once

#include <ostream>
#include <string>
#include <unordered_map>
#include <vector>

#include "movable_object.hpp"
#include "spatial_hash.hpp"

// 探知した相手の情報を保持し、失探判定に利用するための構造体です。
struct DetectionInfo {
    double lat_deg = 0.0;
    double lon_deg = 0.0;
    double alt_m = 0.0;
    int distance_m = 0;
};

// 斥候役の移動と探知を担当するクラスです。
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
                         int self_index,
                         std::ostream &event_out);

private:
    int detect_range_m_ = 0;
    int comm_range_m_ = 0;
    std::unordered_map<std::string, DetectionInfo> detect_state_;
};
