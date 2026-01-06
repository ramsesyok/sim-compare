#include "scout_object.hpp"

#include <cmath>
#include <utility>

#include "detection_event.hpp"
#include "logging.hpp"
#include "geo.hpp"

ScoutObject::ScoutObject(std::string id,
                         std::string team_id,
                         int start_sec,
                         std::vector<RoutePoint> route,
                         std::vector<std::string> network,
                         std::vector<double> segment_end_secs,
                         double total_duration_sec,
                         int detect_range_m,
                         int comm_range_m)
    : MovableObject(std::move(id),
                    std::move(team_id),
                    simoop::Role::SCOUT,
                    start_sec,
                    std::move(route),
                    std::move(network),
                    std::move(segment_end_secs),
                    total_duration_sec),
      m_detect_range_m(detect_range_m),
      m_comm_range_m(comm_range_m) {}

void ScoutObject::updateDetection(
    int time_sec,
    const std::unordered_map<CellKey, std::vector<int>, CellKeyHash> &spatial_hash,
    const std::vector<SimObject *> &objects,
    int self_index) {
    // 探知は斥候の責務としてまとめ、他の役割が関与しないようにします。
    if (m_detect_range_m <= 0) {
        return;
    }

    std::unordered_map<std::string, DetectionInfo> current_detected;
    CellKey base = cellKey(m_position, static_cast<double>(m_detect_range_m));

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dz = -1; dz <= 1; ++dz) {
                CellKey key{base.x + dx, base.y + dy, base.z + dz};
                auto it = spatial_hash.find(key);
                if (it == spatial_hash.end()) {
                    continue;
                }
                for (int index : it->second) {
                    if (index == self_index) {
                        continue;
                    }
                    const SimObject *other = objects[index];
                    if (other->teamId() == m_team_id) {
                        continue;
                    }
                    double distance = distanceEcef(m_position, other->position());
                    if (distance > static_cast<double>(m_detect_range_m)) {
                        continue;
                    }
                    double lat = 0.0;
                    double lon = 0.0;
                    double alt = 0.0;
                    ecefToGeodetic(other->position(), lat, lon, alt);
                    DetectionInfo info;
                    info.lat_deg = lat;
                    info.lon_deg = lon;
                    info.alt_m = alt;
                    info.distance_m = static_cast<int>(std::llround(distance));
                    current_detected.emplace(other->id(), info);
                }
            }
        }
    }

    for (const auto &entry : current_detected) {
        if (m_detect_state.find(entry.first) != m_detect_state.end()) {
            continue;
        }
        const DetectionInfo &info = entry.second;
        simoop::DetectionEvent event;
        event.setEventType("detection");
        event.setDetectionAction(simoop::DetectionAction::FOUND);
        event.setTimeSec(time_sec);
        event.setScountId(m_id);
        event.setLatDeg(info.lat_deg);
        event.setLonDeg(info.lon_deg);
        event.setAltM(info.alt_m);
        event.setDistanceM(info.distance_m);
        event.setDetectId(entry.first);
        nlohmann::json json_event;
        simoop::to_json(json_event, event);
        EventLogger::instance().write(json_event);
    }

    for (const auto &entry : m_detect_state) {
        if (current_detected.find(entry.first) != current_detected.end()) {
            continue;
        }
        const DetectionInfo &info = entry.second;
        simoop::DetectionEvent event;
        event.setEventType("detection");
        event.setDetectionAction(simoop::DetectionAction::LOST);
        event.setTimeSec(time_sec);
        event.setScountId(m_id);
        event.setLatDeg(info.lat_deg);
        event.setLonDeg(info.lon_deg);
        event.setAltM(info.alt_m);
        event.setDistanceM(info.distance_m);
        event.setDetectId(entry.first);
        nlohmann::json json_event;
        simoop::to_json(json_event, event);
        EventLogger::instance().write(json_event);
    }

    m_detect_state = std::move(current_detected);
}
