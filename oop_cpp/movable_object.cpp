#include "movable_object.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

MovableObject::MovableObject(std::string id,
                             std::string team_id,
                             jsonobj::Role role,
                             int start_sec,
                             std::vector<RoutePoint> route,
                             std::vector<std::string> network,
                             std::vector<double> segment_end_secs,
                             double total_duration_sec)
    : SimObject(std::move(id), std::move(team_id), role, start_sec, std::move(route), std::move(network)),
      m_segment_end_secs(std::move(segment_end_secs)),
      m_total_duration_sec(total_duration_sec) {}

void MovableObject::updatePosition(int time_sec) {
    // 司令官は移動しないため、経路の先頭に固定します。
    // 親クラス側で移動判定を共通化することで、派生クラスの実装を簡素にします。
    if (m_role == jsonobj::Role::COMMANDER) {
        if (!m_route.empty()) {
            m_position = m_route.front().ecef;
        }
        return;
    }

    if (m_route.empty()) {
        m_position = Ecef{};
        return;
    }

    // 移動開始時間前は最初の経路点に待機します。
    // 移動の判定はここで完結させ、派生クラスに時間管理を持ち込まない設計です。
    if (time_sec < m_start_sec) {
        m_position = m_route.front().ecef;
        return;
    }

    if (m_segment_end_secs.empty()) {
        m_position = m_route.back().ecef;
        return;
    }

    double elapsed = static_cast<double>(time_sec - m_start_sec);
    if (elapsed >= m_total_duration_sec) {
        m_position = m_route.back().ecef;
        return;
    }

    auto it = std::upper_bound(m_segment_end_secs.begin(), m_segment_end_secs.end(), elapsed);
    size_t segment_index = static_cast<size_t>(std::distance(m_segment_end_secs.begin(), it));
    if (segment_index >= m_segment_end_secs.size()) {
        m_position = m_route.back().ecef;
        return;
    }

    double segment_end = m_segment_end_secs[segment_index];
    double segment_start = (segment_index == 0) ? 0.0 : m_segment_end_secs[segment_index - 1];
    double segment_duration = segment_end - segment_start;
    if (segment_duration <= 0.0) {
        m_position = m_route[segment_index + 1].ecef;
        return;
    }
    if (!std::isfinite(segment_duration)) {
        m_position = m_route[segment_index].ecef;
        return;
    }

    double t = (elapsed - segment_start) / segment_duration;
    const Ecef &a = m_route[segment_index].ecef;
    const Ecef &b = m_route[segment_index + 1].ecef;
    m_position = Ecef{
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
    };
}
