#include "aos_simulation.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

#include "jsonobj/detection_event.hpp"
#include "jsonobj/detonation_event.hpp"
#include "route.hpp"
#include "spatial_hash.hpp"

std::string AosSimulation::roleToString(jsonobj::Role role) const {
    // 役割の文字列化を一箇所にまとめて、ログ出力の表記を統一します。
    switch (role) {
    case jsonobj::Role::COMMANDER:
        return "commander";
    case jsonobj::Role::SCOUT:
        return "scout";
    case jsonobj::Role::MESSENGER:
        return "messenger";
    case jsonobj::Role::ATTACKER:
        return "attacker";
    }
    return "unknown";
}

jsonobj::Scenario AosSimulation::loadScenario(const std::string &path) const {
    // シナリオ読み込みはSimulation内部で完結させ、外部に解析手順を露出しません。
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("scenario: failed to open " + path);
    }
    nlohmann::json data;
    file >> data;
    jsonobj::Scenario scenario;
    jsonobj::from_json(data, scenario);
    return scenario;
}

void AosSimulation::buildStorage(const jsonobj::Scenario &scenario) {
    // シナリオの定義をAoS配列へ展開し、1個体分の情報がまとまるようにします。
    size_t total_objects = 0;
    for (const auto &team : scenario.getTeams()) {
        total_objects += team.getObjects().size();
    }

    m_storage.objects.clear();
    m_storage.objects.reserve(total_objects);

    for (const auto &team : scenario.getTeams()) {
        for (const auto &obj : team.getObjects()) {
            std::vector<RoutePoint> route = buildRoute(obj.getRoute());
            auto segment_info = buildSegmentTimes(route);
            std::vector<double> segment_ends = std::move(segment_info.first);
            double total_duration = segment_info.second;

            AosObject record;
            record.object_id = obj.getId();
            record.team_id = team.getId();
            record.role = obj.getRole();
            record.start_sec = static_cast<int>(obj.getStartSec());
            record.route = std::move(route);
            record.segment_end_secs = std::move(segment_ends);
            record.total_duration_sec = total_duration;
            record.has_detonated = false;

            if (!record.route.empty()) {
                record.position = record.route.front().ecef;
            } else {
                record.position = Ecef{0.0, 0.0, 0.0};
            }

            m_storage.objects.push_back(std::move(record));
        }
    }
}

void AosSimulation::initialize(const std::string &scenario_path,
                               const std::string &timeline_path,
                               const std::string &event_path) {
    // AoS(Array of Structures)では、1個体の状態を1つの構造体にまとめます。
    // これにより「個体ごとの更新処理」が読みやすくなり、状態のまとまりを把握しやすくなります。
    // initializeでは準備に集中し、runではループ本体だけを実行する構成にしています。
    m_event_logger.open(event_path);
    m_timeline_logger.open(timeline_path);
    m_scenario = loadScenario(scenario_path);
    buildStorage(m_scenario);
    m_end_sec = 24 * 60 * 60;
    m_detect_range_m = static_cast<int>(m_scenario.getPerformance().getScout().getDetectRangeM());
    m_comm_range_m = static_cast<int>(m_scenario.getPerformance().getScout().getCommRangeM());
    m_bom_range_m = static_cast<int>(m_scenario.getPerformance().getAttacker().getBomRangeM());
    m_initialized = true;
}

void AosSimulation::run() {
    if (!m_initialized) {
        throw std::runtime_error("simulation: initialize must be called before run");
    }

    // AoSでは「1個体のまとまり」を順番に更新し、イベント判定とログ出力を行います。
    for (int time_sec = 0; time_sec <= m_end_sec; ++time_sec) {
        updatePositions(time_sec);

        std::unordered_map<CellKey, std::vector<int>, CellKeyHash> spatial_hash =
            buildSpatialHash(m_storage, static_cast<double>(m_detect_range_m));

        for (size_t i = 0; i < m_storage.objects.size(); ++i) {
            if (m_storage.objects[i].role == jsonobj::Role::SCOUT) {
                updateDetectionForScout(time_sec, i, spatial_hash);
            }
        }

        for (size_t i = 0; i < m_storage.objects.size(); ++i) {
            if (m_storage.objects[i].role == jsonobj::Role::ATTACKER) {
                emitDetonationForAttacker(time_sec, i);
            }
        }

        m_timeline_logger.write(time_sec, m_storage, *this);
    }
}

void AosSimulation::updatePositions(int time_sec) {
    // AoSでは個体単位で状態を更新するため、1件ずつ読みやすく処理できます。
    for (auto &obj : m_storage.objects) {
        if (obj.route.empty()) {
            obj.position = Ecef{0.0, 0.0, 0.0};
            continue;
        }

        const RoutePoint &first = obj.route.front();

        if (obj.role == jsonobj::Role::COMMANDER) {
            obj.position = first.ecef;
            continue;
        }

        if (time_sec < obj.start_sec) {
            obj.position = first.ecef;
            continue;
        }

        if (obj.segment_end_secs.empty()) {
            obj.position = obj.route.back().ecef;
            continue;
        }

        double elapsed = static_cast<double>(time_sec - obj.start_sec);
        if (elapsed >= obj.total_duration_sec) {
            obj.position = obj.route.back().ecef;
            continue;
        }

        auto it = std::upper_bound(obj.segment_end_secs.begin(), obj.segment_end_secs.end(), elapsed);
        size_t segment_index = static_cast<size_t>(std::distance(obj.segment_end_secs.begin(), it));
        if (segment_index >= obj.segment_end_secs.size()) {
            obj.position = obj.route.back().ecef;
            continue;
        }

        double segment_end = obj.segment_end_secs[segment_index];
        double segment_start = (segment_index == 0) ? 0.0 : obj.segment_end_secs[segment_index - 1];
        double segment_duration = segment_end - segment_start;
        if (segment_duration <= 0.0) {
            obj.position = obj.route[segment_index + 1].ecef;
            continue;
        }
        if (!std::isfinite(segment_duration)) {
            obj.position = obj.route[segment_index].ecef;
            continue;
        }

        double t = (elapsed - segment_start) / segment_duration;
        const RoutePoint &a = obj.route[segment_index];
        const RoutePoint &b = obj.route[segment_index + 1];
        obj.position = Ecef{
            a.ecef.x + (b.ecef.x - a.ecef.x) * t,
            a.ecef.y + (b.ecef.y - a.ecef.y) * t,
            a.ecef.z + (b.ecef.z - a.ecef.z) * t,
        };
    }
}

void AosSimulation::updateDetectionForScout(
    int time_sec,
    size_t scout_index,
    const std::unordered_map<CellKey, std::vector<int>, CellKeyHash> &spatial_hash) {
    // 探知範囲が無効なら処理を省略し、無駄な計算を避けます。
    if (m_detect_range_m <= 0) {
        return;
    }

    AosObject &scout = m_storage.objects[scout_index];
    std::unordered_map<std::string, DetectionInfo> current_detected;
    CellKey base = cellKey(scout.position, static_cast<double>(m_detect_range_m));

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dz = -1; dz <= 1; ++dz) {
                CellKey key{base.x + dx, base.y + dy, base.z + dz};
                auto it = spatial_hash.find(key);
                if (it == spatial_hash.end()) {
                    continue;
                }
                for (int index : it->second) {
                    size_t other_index = static_cast<size_t>(index);
                    if (other_index == scout_index) {
                        continue;
                    }
                    const AosObject &other = m_storage.objects[other_index];
                    if (other.team_id == scout.team_id) {
                        continue;
                    }
                    double distance = distanceEcef(scout.position, other.position);
                    if (distance > static_cast<double>(m_detect_range_m)) {
                        continue;
                    }
                    double lat = 0.0;
                    double lon = 0.0;
                    double alt = 0.0;
                    ecefToGeodetic(other.position, lat, lon, alt);
                    DetectionInfo info;
                    info.lat_deg = lat;
                    info.lon_deg = lon;
                    info.alt_m = alt;
                    info.distance_m = static_cast<int>(std::llround(distance));
                    current_detected.emplace(other.object_id, info);
                }
            }
        }
    }

    for (const auto &entry : current_detected) {
        if (scout.detect_state.find(entry.first) != scout.detect_state.end()) {
            continue;
        }
        const DetectionInfo &info = entry.second;
        jsonobj::DetectionEvent event;
        event.setEventType("detection");
        event.setDetectionAction(jsonobj::DetectionAction::FOUND);
        event.setTimeSec(time_sec);
        event.setScountId(scout.object_id);
        event.setLatDeg(info.lat_deg);
        event.setLonDeg(info.lon_deg);
        event.setAltM(info.alt_m);
        event.setDistanceM(info.distance_m);
        event.setDetectId(entry.first);
        nlohmann::json json_event;
        jsonobj::to_json(json_event, event);
        m_event_logger.write(json_event);
    }

    for (const auto &entry : scout.detect_state) {
        if (current_detected.find(entry.first) != current_detected.end()) {
            continue;
        }
        const DetectionInfo &info = entry.second;
        jsonobj::DetectionEvent event;
        event.setEventType("detection");
        event.setDetectionAction(jsonobj::DetectionAction::LOST);
        event.setTimeSec(time_sec);
        event.setScountId(scout.object_id);
        event.setLatDeg(info.lat_deg);
        event.setLonDeg(info.lon_deg);
        event.setAltM(info.alt_m);
        event.setDistanceM(info.distance_m);
        event.setDetectId(entry.first);
        nlohmann::json json_event;
        jsonobj::to_json(json_event, event);
        m_event_logger.write(json_event);
    }

    scout.detect_state = std::move(current_detected);
}

void AosSimulation::emitDetonationForAttacker(int time_sec, size_t attacker_index) {
    // 爆破イベントは攻撃役の責務として扱い、1回だけ発火させます。
    AosObject &attacker = m_storage.objects[attacker_index];
    if (attacker.has_detonated) {
        return;
    }
    if (!std::isfinite(attacker.total_duration_sec)) {
        return;
    }
    if (static_cast<double>(time_sec) <
        static_cast<double>(attacker.start_sec) + attacker.total_duration_sec) {
        return;
    }

    double lat = 0.0;
    double lon = 0.0;
    double alt = 0.0;
    ecefToGeodetic(attacker.position, lat, lon, alt);
    jsonobj::DetonationEvent event;
    event.setEventType("detonation");
    event.setTimeSec(time_sec);
    event.setAttackerId(attacker.object_id);
    event.setLatDeg(lat);
    event.setLonDeg(lon);
    event.setAltM(alt);
    event.setBomRangeM(m_bom_range_m);
    nlohmann::json json_event;
    jsonobj::to_json(json_event, event);
    m_event_logger.write(json_event);
    attacker.has_detonated = true;
}
