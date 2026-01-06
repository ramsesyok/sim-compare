#include "attacker_object.hpp"

#include <cmath>
#include <utility>

#include "detonation_event.hpp"
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
      bom_range_m_(bom_range_m) {}

void AttackerObject::emitDetonation(int time_sec, std::ostream &event_out) {
    if (has_detonated_) {
        return;
    }
    if (!std::isfinite(total_duration_sec_)) {
        return;
    }
    if (static_cast<double>(time_sec) < static_cast<double>(start_sec_) + total_duration_sec_) {
        return;
    }

    double lat = 0.0;
    double lon = 0.0;
    double alt = 0.0;
    ecefToGeodetic(position_, lat, lon, alt);
    simoop::DetonationEvent event;
    event.setEventType("detonation");
    event.setTimeSec(time_sec);
    event.setAttackerId(id_);
    event.setLatDeg(lat);
    event.setLonDeg(lon);
    event.setAltM(alt);
    event.setBomRangeM(bom_range_m_);
    nlohmann::json json_event;
    simoop::to_json(json_event, event);
    event_out << json_event.dump() << '\n';
    has_detonated_ = true;
}
