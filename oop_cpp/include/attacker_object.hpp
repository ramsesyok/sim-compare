#pragma once

#include "movable_object.hpp"

class EventLogger;

/**
 * @brief 攻撃役の移動と爆破ログの出力を扱うクラスです。
 *
 * @details MovableObjectの移動に加えて「到達時の爆破」という責務を追加する設計です。
 */
class AttackerObject : public MovableObject {
public:
    /**
     * @brief 爆破範囲を含めて初期化します。
     */
    AttackerObject(std::string id,
                   std::string team_id,
                   int start_sec,
                   std::vector<RoutePoint> route,
                   std::vector<std::string> network,
                   std::vector<double> segment_end_secs,
                   double total_duration_sec,
                   int bom_range_m,
                   EventLogger *event_logger);

    /**
     * @brief 到達後に一度だけ爆破イベントを出力します。
     */
    void emitDetonation(int time_sec);

private:
    int m_bom_range_m = 0;
    bool m_has_detonated = false;
    EventLogger *m_event_logger = nullptr;
};
