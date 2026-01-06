#pragma once

#include <ostream>

#include "movable_object.hpp"

// 攻撃役の移動と爆破ログの出力を扱うクラスです。
class AttackerObject : public MovableObject {
public:
    AttackerObject(std::string id,
                   std::string team_id,
                   int start_sec,
                   std::vector<RoutePoint> route,
                   std::vector<std::string> network,
                   std::vector<double> segment_end_secs,
                   double total_duration_sec,
                   int bom_range_m);

    void emitDetonation(int time_sec, std::ostream &event_out);

private:
    int bom_range_m_ = 0;
    bool has_detonated_ = false;
};
