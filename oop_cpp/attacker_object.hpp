#pragma once

#include "movable_object.hpp"

// 攻撃役の移動と爆破ログの出力を扱うクラスです。
// MovableObjectの移動に加えて「到達時の爆破」という責務を追加する設計です。
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

    void emitDetonation(int time_sec);

private:
    int m_bom_range_m = 0;
    bool m_has_detonated = false;
};
