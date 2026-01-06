#pragma once

#include "sim_object.hpp"

// 経路に沿って移動するオブジェクトの基底クラスで、移動の補間処理を持ちます。
// 共通の移動ロジックを親に置き、斥候・伝令・攻撃の違いは派生クラスで追加します。
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
    std::vector<double> m_segment_end_secs;
    double m_total_duration_sec = 0.0;
};
