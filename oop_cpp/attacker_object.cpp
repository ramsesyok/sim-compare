#include "attacker_object.hpp"

#include <cmath>
#include <utility>

#include "detonation_event.hpp"
#include "logging.hpp"
#include "geo.hpp"

AttackerObject::AttackerObject(std::string id,
                               std::string team_id,
                               int start_sec,
                               std::vector<RoutePoint> route,
                               std::vector<std::string> network,
                               std::vector<double> segment_end_secs,
                               double total_duration_sec,
                               int bom_range_m)
    : MovableObject(std::move(id),
                    std::move(team_id),
                    simoop::Role::ATTACKER,
                    start_sec,
                    std::move(route),
                    std::move(network),
                    std::move(segment_end_secs),
                    total_duration_sec),
      m_bom_range_m(bom_range_m) {}

void AttackerObject::emitDetonation(int time_sec, EventLogger &event_logger) {
    // 爆破イベントは攻撃役の責務として扱い、他クラスには波及させません。
    if (m_has_detonated) {
        return;
    }
    if (!std::isfinite(m_total_duration_sec)) {
        return;
    }
    if (static_cast<double>(time_sec) < static_cast<double>(m_start_sec) + m_total_duration_sec) {
        return;
    }

    double lat = 0.0;
    double lon = 0.0;
    double alt = 0.0;
    ecefToGeodetic(m_position, lat, lon, alt);
    simoop::DetonationEvent event;
    event.setEventType("detonation");
    event.setTimeSec(time_sec);
    event.setAttackerId(m_id);
    event.setLatDeg(lat);
    event.setLonDeg(lon);
    event.setAltM(alt);
    event.setBomRangeM(m_bom_range_m);
    nlohmann::json json_event;
    simoop::to_json(json_event, event);
    event_logger.write(json_event);
    m_has_detonated = true;
}
