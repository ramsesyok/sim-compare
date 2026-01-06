#pragma once

#include "sim_object.hpp"

// 経路に沿って移動するオブジェクトの基底クラスで、移動の補間処理を持ちます。
class MovableObject : public SimObject {
public:
    MovableObject(std::string id,
                  std::string team_id,
                  simoop::Role role,
                  int start_sec,
                  std::vector<RoutePoint> route,
                  std::vector<std::string> network,
                  std::vector<double> segment_end_secs,
                  double total_duration_sec);

    void updatePosition(int time_sec) override;

protected:
    std::vector<double> segment_end_secs_;
    double total_duration_sec_ = 0.0;
};
