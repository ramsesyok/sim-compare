#include "simulation.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <unordered_map>

#include "jsonobj/detection_event.hpp"
#include "jsonobj/detonation_event.hpp"
#include "route.hpp"
#include "spatial_hash.hpp"

std::string Simulation::roleToString(jsonobj::Role role) const {
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

jsonobj::Scenario Simulation::loadScenario(const std::string &path) const {
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

void Simulation::buildStorage(const jsonobj::Scenario &scenario) {
    // シナリオの定義をSoA配列へ展開し、後続の更新処理で連続メモリ参照を活用します。
    size_t total_objects = 0;
    for (const auto &team : scenario.getTeams()) {
        total_objects += team.getObjects().size();
    }

    m_storage.object_ids.clear();
    m_storage.team_ids.clear();
    m_storage.roles.clear();
    m_storage.start_secs.clear();
    m_storage.ecef_xs.clear();
    m_storage.ecef_ys.clear();
    m_storage.ecef_zs.clear();
    m_storage.route_points.clear();
    m_storage.route_offsets.clear();
    m_storage.route_counts.clear();
    m_storage.segment_end_secs.clear();
    m_storage.segment_offsets.clear();
    m_storage.segment_counts.clear();
    m_storage.total_duration_secs.clear();
    m_storage.detect_states.clear();
    m_storage.has_detonated.clear();

    m_storage.object_ids.reserve(total_objects);
    m_storage.team_ids.reserve(total_objects);
    m_storage.roles.reserve(total_objects);
    m_storage.start_secs.reserve(total_objects);
    m_storage.ecef_xs.reserve(total_objects);
    m_storage.ecef_ys.reserve(total_objects);
    m_storage.ecef_zs.reserve(total_objects);
    m_storage.route_offsets.reserve(total_objects);
    m_storage.route_counts.reserve(total_objects);
    m_storage.segment_offsets.reserve(total_objects);
    m_storage.segment_counts.reserve(total_objects);
    m_storage.total_duration_secs.reserve(total_objects);
    m_storage.detect_states.reserve(total_objects);
    m_storage.has_detonated.reserve(total_objects);

    for (const auto &team : scenario.getTeams()) {
        for (const auto &obj : team.getObjects()) {
            std::vector<RoutePoint> route = buildRoute(obj.getRoute());
            auto segment_info = buildSegmentTimes(route);
            std::vector<double> segment_ends = std::move(segment_info.first);
            double total_duration = segment_info.second;

            size_t route_offset = m_storage.route_points.size();
            size_t route_count = route.size();
            m_storage.route_points.insert(
                m_storage.route_points.end(),
                route.begin(),
                route.end());
            m_storage.route_offsets.push_back(route_offset);
            m_storage.route_counts.push_back(route_count);

            size_t segment_offset = m_storage.segment_end_secs.size();
            size_t segment_count = segment_ends.size();
            m_storage.segment_end_secs.insert(
                m_storage.segment_end_secs.end(),
                segment_ends.begin(),
                segment_ends.end());
            m_storage.segment_offsets.push_back(segment_offset);
            m_storage.segment_counts.push_back(segment_count);
            m_storage.total_duration_secs.push_back(total_duration);

            m_storage.object_ids.push_back(obj.getId());
            m_storage.team_ids.push_back(team.getId());
            m_storage.roles.push_back(obj.getRole());
            m_storage.start_secs.push_back(static_cast<int>(obj.getStartSec()));

            if (!route.empty()) {
                const auto &first = route.front();
                m_storage.ecef_xs.push_back(first.ecef.x);
                m_storage.ecef_ys.push_back(first.ecef.y);
                m_storage.ecef_zs.push_back(first.ecef.z);
            } else {
                m_storage.ecef_xs.push_back(0.0);
                m_storage.ecef_ys.push_back(0.0);
                m_storage.ecef_zs.push_back(0.0);
            }

            m_storage.detect_states.emplace_back();
            m_storage.has_detonated.push_back(false);
        }
    }
}

void Simulation::initialize(const std::string &scenario_path,
                            const std::string &timeline_path,
                            const std::string &event_path) {
    // SoA(Structure of Arrays)では、属性ごとの配列にデータを並べて管理します。
    // そのため「位置だけ更新する」「通信範囲だけ判定する」といった処理を
    // 連続メモリで高速に行いやすく、シミュレーションの比較検証に役立ちます。
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

void Simulation::run() {
    if (!m_initialized) {
        throw std::runtime_error("simulation: initialize must be called before run");
    }

    // SoAでは属性ごとの配列を連続走査できるため、1秒ごとの更新を効率的に書けます。
    // ここでは1秒刻みで配列を順に走査しながら更新とログ出力を行います。
    for (int time_sec = 0; time_sec <= m_end_sec; ++time_sec) {
        std::vector<Ecef> positions = updatePositions(m_storage, time_sec);
        if (positions.size() != m_storage.object_ids.size()) {
            throw std::runtime_error("simulation: position count mismatch");
        }
        for (size_t i = 0; i < positions.size(); ++i) {
            m_storage.ecef_xs[i] = positions[i].x;
            m_storage.ecef_ys[i] = positions[i].y;
            m_storage.ecef_zs[i] = positions[i].z;
        }

        std::unordered_map<CellKey, std::vector<int>, CellKeyHash> spatial_hash =
            buildSpatialHash(m_storage, static_cast<double>(m_detect_range_m));

        for (size_t i = 0; i < m_storage.object_ids.size(); ++i) {
            if (m_storage.roles[i] == jsonobj::Role::SCOUT) {
                updateDetectionForScout(time_sec, i, spatial_hash);
            }
        }

        for (size_t i = 0; i < m_storage.object_ids.size(); ++i) {
            if (m_storage.roles[i] == jsonobj::Role::ATTACKER) {
                emitDetonationForAttacker(time_sec, i);
            }
        }
        m_timeline_logger.write(time_sec, m_storage, *this);
    }
}

std::vector<Ecef> Simulation::updatePositions(const SoaStorage &storage, int time_sec) const {
    // SoA配列の入力から位置を計算し、副作用なしで結果を返します。
    // これにより同じ入力なら常に同じ出力になるため、挙動の追跡が容易になります。
    // SoAの利点は「属性ごとの配列を連続して走査できること」です。
    // まとめて走査するとCPUキャッシュに乗りやすく、個別オブジェクトを点在アクセスするAoSよりも
    // メモリアクセスが素直になります。学習用として、このループは関数内で完結させます。
    // また、分岐(開始前・終了後・区間内など)は同じ順序で並ぶため、CPUの分岐予測が外れにくくなり、
    // 1件ずつ関数を呼ぶよりも安定した実行時間になりやすいことを意識しています。
    std::vector<Ecef> positions;
    positions.resize(storage.object_ids.size());

    for (size_t i = 0; i < storage.object_ids.size(); ++i) {
        size_t route_count = storage.route_counts[i];
        if (route_count == 0) {
            positions[i] = Ecef{0.0, 0.0, 0.0};
            continue;
        }

        size_t route_offset = storage.route_offsets[i];
        const RoutePoint &first = storage.route_points[route_offset];

        if (storage.roles[i] == jsonobj::Role::COMMANDER) {
            positions[i] = first.ecef;
            continue;
        }

        if (time_sec < storage.start_secs[i]) {
            positions[i] = first.ecef;
            continue;
        }

        size_t segment_count = storage.segment_counts[i];
        if (segment_count == 0) {
            const RoutePoint &last = storage.route_points[route_offset + route_count - 1];
            positions[i] = last.ecef;
            continue;
        }

        double elapsed = static_cast<double>(time_sec - storage.start_secs[i]);
        double total_duration = storage.total_duration_secs[i];
        if (elapsed >= total_duration) {
            const RoutePoint &last = storage.route_points[route_offset + route_count - 1];
            positions[i] = last.ecef;
            continue;
        }

        size_t segment_offset = storage.segment_offsets[i];
        auto begin = storage.segment_end_secs.begin() +
                     static_cast<std::ptrdiff_t>(segment_offset);
        auto end = begin + static_cast<std::ptrdiff_t>(segment_count);
        auto it = std::upper_bound(begin, end, elapsed);
        size_t segment_index = static_cast<size_t>(std::distance(begin, it));

        if (segment_index >= segment_count) {
            const RoutePoint &last = storage.route_points[route_offset + route_count - 1];
            positions[i] = last.ecef;
            continue;
        }

        double segment_end = storage.segment_end_secs[segment_offset + segment_index];
        double segment_start = (segment_index == 0)
                                   ? 0.0
                                   : storage.segment_end_secs[segment_offset + segment_index - 1];
        double segment_duration = segment_end - segment_start;
        if (segment_duration <= 0.0) {
            const RoutePoint &target = storage.route_points[route_offset + segment_index + 1];
            positions[i] = target.ecef;
            continue;
        }
        if (!std::isfinite(segment_duration)) {
            const RoutePoint &target = storage.route_points[route_offset + segment_index];
            positions[i] = target.ecef;
            continue;
        }

        double t = (elapsed - segment_start) / segment_duration;
        const RoutePoint &a = storage.route_points[route_offset + segment_index];
        const RoutePoint &b = storage.route_points[route_offset + segment_index + 1];
        positions[i] = Ecef{
            a.ecef.x + (b.ecef.x - a.ecef.x) * t,
            a.ecef.y + (b.ecef.y - a.ecef.y) * t,
            a.ecef.z + (b.ecef.z - a.ecef.z) * t,
        };
    }

    return positions;
}

void Simulation::updateDetectionForScout(
    int time_sec,
    size_t scout_index,
    const std::unordered_map<CellKey, std::vector<int>, CellKeyHash> &spatial_hash) {
    // 探知範囲が無効なら処理を省略し、無駄な計算を避けます。
    if (m_detect_range_m <= 0) {
        return;
    }

    std::unordered_map<std::string, DetectionInfo> current_detected;
    Ecef scout_pos{m_storage.ecef_xs[scout_index], m_storage.ecef_ys[scout_index], m_storage.ecef_zs[scout_index]};
    CellKey base = cellKey(scout_pos, static_cast<double>(m_detect_range_m));

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
                    if (m_storage.team_ids[other_index] == m_storage.team_ids[scout_index]) {
                        continue;
                    }
                    Ecef other_pos{
                        m_storage.ecef_xs[other_index],
                        m_storage.ecef_ys[other_index],
                        m_storage.ecef_zs[other_index],
                    };
                    double distance = distanceEcef(scout_pos, other_pos);
                    if (distance > static_cast<double>(m_detect_range_m)) {
                        continue;
                    }
                    double lat = 0.0;
                    double lon = 0.0;
                    double alt = 0.0;
                    ecefToGeodetic(other_pos, lat, lon, alt);
                    DetectionInfo info;
                    info.lat_deg = lat;
                    info.lon_deg = lon;
                    info.alt_m = alt;
                    info.distance_m = static_cast<int>(std::llround(distance));
                    current_detected.emplace(m_storage.object_ids[other_index], info);
                }
            }
        }
    }

    auto &previous_detected = m_storage.detect_states[scout_index];
    for (const auto &entry : current_detected) {
        if (previous_detected.find(entry.first) != previous_detected.end()) {
            continue;
        }
        const DetectionInfo &info = entry.second;
        jsonobj::DetectionEvent event;
        event.setEventType("detection");
        event.setDetectionAction(jsonobj::DetectionAction::FOUND);
        event.setTimeSec(time_sec);
        event.setScountId(m_storage.object_ids[scout_index]);
        event.setLatDeg(info.lat_deg);
        event.setLonDeg(info.lon_deg);
        event.setAltM(info.alt_m);
        event.setDistanceM(info.distance_m);
        event.setDetectId(entry.first);
        nlohmann::json json_event;
        jsonobj::to_json(json_event, event);
        m_event_logger.write(json_event);
    }

    for (const auto &entry : previous_detected) {
        if (current_detected.find(entry.first) != current_detected.end()) {
            continue;
        }
        const DetectionInfo &info = entry.second;
        jsonobj::DetectionEvent event;
        event.setEventType("detection");
        event.setDetectionAction(jsonobj::DetectionAction::LOST);
        event.setTimeSec(time_sec);
        event.setScountId(m_storage.object_ids[scout_index]);
        event.setLatDeg(info.lat_deg);
        event.setLonDeg(info.lon_deg);
        event.setAltM(info.alt_m);
        event.setDistanceM(info.distance_m);
        event.setDetectId(entry.first);
        nlohmann::json json_event;
        jsonobj::to_json(json_event, event);
        m_event_logger.write(json_event);
    }

    previous_detected = std::move(current_detected);
}

void Simulation::emitDetonationForAttacker(int time_sec, size_t attacker_index) {
    // 爆破イベントは攻撃役の責務として扱い、1回だけ発火させます。
    if (m_storage.has_detonated[attacker_index]) {
        return;
    }
    double total_duration = m_storage.total_duration_secs[attacker_index];
    if (!std::isfinite(total_duration)) {
        return;
    }
    if (static_cast<double>(time_sec) <
        static_cast<double>(m_storage.start_secs[attacker_index]) + total_duration) {
        return;
    }

    Ecef pos{
        m_storage.ecef_xs[attacker_index],
        m_storage.ecef_ys[attacker_index],
        m_storage.ecef_zs[attacker_index],
    };
    double lat = 0.0;
    double lon = 0.0;
    double alt = 0.0;
    ecefToGeodetic(pos, lat, lon, alt);
    jsonobj::DetonationEvent event;
    event.setEventType("detonation");
    event.setTimeSec(time_sec);
    event.setAttackerId(m_storage.object_ids[attacker_index]);
    event.setLatDeg(lat);
    event.setLonDeg(lon);
    event.setAltM(alt);
    event.setBomRangeM(m_bom_range_m);
    nlohmann::json json_event;
    jsonobj::to_json(json_event, event);
    m_event_logger.write(json_event);
    m_storage.has_detonated[attacker_index] = true;
}
