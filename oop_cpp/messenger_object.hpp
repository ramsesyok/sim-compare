#pragma once

#include "movable_object.hpp"

// 伝令役の移動を扱うクラスで、通信動作は今後の拡張に備えて保持します。
// 移動ロジックは親クラスに任せ、役割固有の属性だけをこのクラスで持ちます。
class MessengerObject : public MovableObject {
public:
    MessengerObject(std::string id,
                    std::string team_id,
                    int start_sec,
                    std::vector<RoutePoint> route,
                    std::vector<std::string> network,
                    std::vector<double> segment_end_secs,
                    double total_duration_sec,
                    int comm_range_m);

private:
    int m_comm_range_m = 0;
};
