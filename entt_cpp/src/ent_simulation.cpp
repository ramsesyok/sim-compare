#include "ent_simulation.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_map>

#include "ecs_components.hpp"
#include "jsonobj/detection_event.hpp"
#include "jsonobj/detonation_event.hpp"
#include "route.hpp"
#include "spatial_hash.hpp"

std::string EnttSimulation::roleToString(jsonobj::Role role) const {
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

jsonobj::Scenario EnttSimulation::loadScenario(const std::string &path) const {
    // シナリオ読み込みはEnttSimulation内部で完結させ、外部に解析手順を露出しません。
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

void EnttSimulation::buildRegistry(const jsonobj::Scenario &scenario) {
    // ECSではエンティティに必要なコンポーネントだけを付与します。
    // ここでレジストリを組み立てておくと、run中の処理が単純になります。
    m_registry.clear();
    m_entities.clear();

    for (const auto &team : scenario.getTeams()) {
        for (const auto &obj : team.getObjects()) {
            std::vector<RoutePoint> route = buildRoute(obj.getRoute());
            auto segment_info = buildSegmentTimes(route);

            RouteComponent route_component;
            route_component.points = std::move(route);
            route_component.segment_end_secs = std::move(segment_info.first);
            route_component.total_duration = segment_info.second;

            Ecef start_ecef{0.0, 0.0, 0.0};
            if (!route_component.points.empty()) {
                start_ecef = route_component.points.front().ecef;
            }

            entt::entity entity = m_registry.create();
            m_entities.push_back(entity);
            m_registry.emplace<ObjectIdComponent>(entity, obj.getId());
            m_registry.emplace<TeamIdComponent>(entity, team.getId());
            m_registry.emplace<RoleComponent>(entity, obj.getRole());
            m_registry.emplace<StartSecComponent>(entity, static_cast<int>(obj.getStartSec()));
            m_registry.emplace<PositionComponent>(entity, PositionComponent{start_ecef});
            m_registry.emplace<RouteComponent>(entity, std::move(route_component));

            if (obj.getRole() == jsonobj::Role::SCOUT) {
                m_registry.emplace<DetectionStateComponent>(entity);
            }
            if (obj.getRole() == jsonobj::Role::ATTACKER) {
                m_registry.emplace<DetonationStateComponent>(entity);
            }
        }
    }
}

void EnttSimulation::initialize(const std::string &scenario_path,
                                const std::string &timeline_path,
                                const std::string &event_path) {
    // ECS(EnTT)では「エンティティに必要なコンポーネントだけを付ける」ことで、
    // 処理対象を絞り込みやすくします。ここではシナリオからエンティティを生成し、
    // runでは「位置更新」「探知」「爆破」などの処理を役割ごとに分けて実行します。
    // initializeは準備だけに集中し、runは毎秒の更新ループに専念させます。
    m_event_logger.open(event_path);
    m_timeline_logger.open(timeline_path);
    m_scenario = loadScenario(scenario_path);
    buildRegistry(m_scenario);
    m_end_sec = 24 * 60 * 60;
    m_detect_range_m = static_cast<int>(m_scenario.getPerformance().getScout().getDetectRangeM());
    m_comm_range_m = static_cast<int>(m_scenario.getPerformance().getScout().getCommRangeM());
    m_bom_range_m = static_cast<int>(m_scenario.getPerformance().getAttacker().getBomRangeM());
    m_initialized = true;
}

void EnttSimulation::run() {
    if (!m_initialized) {
        throw std::runtime_error("simulation: initialize must be called before run");
    }

    // ECSでは役割ごとのシステムに処理を分けるため、
    // 1秒ごとの更新も「位置更新」「探知」「爆破」の順で明示的に呼び出します。
    // EnTTのレジストリはコンポーネント集合をまとめて扱えるため、
    // ここではエンティティ一覧を基準に順序を固定して処理します。
    for (int time_sec = 0; time_sec <= m_end_sec; ++time_sec) {
        updatePositions(time_sec);

        std::unordered_map<CellKey, std::vector<entt::entity>, CellKeyHash> spatial_hash =
            buildSpatialHash(m_registry, m_entities, static_cast<double>(m_detect_range_m));

        for (entt::entity entity : m_entities) {
            const auto &role = m_registry.get<RoleComponent>(entity);
            if (role.value == jsonobj::Role::SCOUT) {
                updateDetectionForScout(time_sec, entity, spatial_hash);
            }
        }

        for (entt::entity entity : m_entities) {
            const auto &role = m_registry.get<RoleComponent>(entity);
            if (role.value == jsonobj::Role::ATTACKER) {
                emitDetonationForAttacker(time_sec, entity);
            }
        }
        m_timeline_logger.write(time_sec, m_registry, m_entities, *this);
    }
}

void EnttSimulation::updatePositions(int time_sec) {
    // ECSでも位置更新は共通のシステムとして扱い、全エンティティを一括で更新します。
    // これにより、ルート補間の計算を同じ関数に集約し、挙動の追跡を簡単にします。
    for (entt::entity entity : m_entities) {
        const auto &role = m_registry.get<RoleComponent>(entity);
        const auto &start = m_registry.get<StartSecComponent>(entity);
        const auto &route = m_registry.get<RouteComponent>(entity);
        auto &pos = m_registry.get<PositionComponent>(entity);

        size_t route_count = route.points.size();
        if (route_count == 0) {
            pos.ecef = Ecef{0.0, 0.0, 0.0};
            continue;
        }

        const RoutePoint &first = route.points.front();

        if (role.value == jsonobj::Role::COMMANDER) {
            pos.ecef = first.ecef;
            continue;
        }

        if (time_sec < start.value) {
            pos.ecef = first.ecef;
            continue;
        }

        size_t segment_count = route.segment_end_secs.size();
        if (segment_count == 0) {
            const RoutePoint &last = route.points.back();
            pos.ecef = last.ecef;
            continue;
        }

        double elapsed = static_cast<double>(time_sec - start.value);
        double total_duration = route.total_duration;
        if (elapsed >= total_duration) {
            const RoutePoint &last = route.points.back();
            pos.ecef = last.ecef;
            continue;
        }

        auto it = std::upper_bound(route.segment_end_secs.begin(),
                                   route.segment_end_secs.end(),
                                   elapsed);
        size_t segment_index = static_cast<size_t>(std::distance(route.segment_end_secs.begin(), it));

        if (segment_index >= segment_count) {
            const RoutePoint &last = route.points.back();
            pos.ecef = last.ecef;
            continue;
        }

        double segment_end = route.segment_end_secs[segment_index];
        double segment_start = (segment_index == 0)
                                   ? 0.0
                                   : route.segment_end_secs[segment_index - 1];
        double segment_duration = segment_end - segment_start;
        if (segment_duration <= 0.0) {
            const RoutePoint &target = route.points[segment_index + 1];
            pos.ecef = target.ecef;
            continue;
        }
        if (!std::isfinite(segment_duration)) {
            const RoutePoint &target = route.points[segment_index];
            pos.ecef = target.ecef;
            continue;
        }

        double t = (elapsed - segment_start) / segment_duration;
        const RoutePoint &a = route.points[segment_index];
        const RoutePoint &b = route.points[segment_index + 1];
        pos.ecef = Ecef{
            a.ecef.x + (b.ecef.x - a.ecef.x) * t,
            a.ecef.y + (b.ecef.y - a.ecef.y) * t,
            a.ecef.z + (b.ecef.z - a.ecef.z) * t,
        };
    }
}

void EnttSimulation::updateDetectionForScout(
    int time_sec,
    entt::entity scout_entity,
    const std::unordered_map<CellKey, std::vector<entt::entity>, CellKeyHash> &spatial_hash) {
    // 探知範囲が無効なら処理を省略し、無駄な計算を避けます。
    if (m_detect_range_m <= 0) {
        return;
    }

    std::unordered_map<std::string, DetectionInfo> current_detected;
    const auto &scout_pos = m_registry.get<PositionComponent>(scout_entity).ecef;
    const auto &scout_team = m_registry.get<TeamIdComponent>(scout_entity).value;
    const auto &scout_id = m_registry.get<ObjectIdComponent>(scout_entity).value;
    CellKey base = cellKey(scout_pos, static_cast<double>(m_detect_range_m));

    for (int dx = -1; dx <= 1; ++dx) {
        for (int dy = -1; dy <= 1; ++dy) {
            for (int dz = -1; dz <= 1; ++dz) {
                CellKey key{base.x + dx, base.y + dy, base.z + dz};
                auto it = spatial_hash.find(key);
                if (it == spatial_hash.end()) {
                    continue;
                }
                for (entt::entity other_entity : it->second) {
                    if (other_entity == scout_entity) {
                        continue;
                    }
                    const auto &other_team = m_registry.get<TeamIdComponent>(other_entity).value;
                    if (other_team == scout_team) {
                        continue;
                    }
                    const auto &other_pos = m_registry.get<PositionComponent>(other_entity).ecef;
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
                    const auto &other_id = m_registry.get<ObjectIdComponent>(other_entity).value;
                    current_detected.emplace(other_id, info);
                }
            }
        }
    }

    auto &previous_detected = m_registry.get<DetectionStateComponent>(scout_entity).detected;
    for (const auto &entry : current_detected) {
        if (previous_detected.find(entry.first) != previous_detected.end()) {
            continue;
        }
        const DetectionInfo &info = entry.second;
        jsonobj::DetectionEvent event;
        event.setEventType("detection");
        event.setDetectionAction(jsonobj::DetectionAction::FOUND);
        event.setTimeSec(time_sec);
        event.setScountId(scout_id);
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
        event.setScountId(scout_id);
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

void EnttSimulation::emitDetonationForAttacker(int time_sec, entt::entity attacker_entity) {
    // 爆破イベントは攻撃役の責務として扱い、1回だけ発火させます。
    auto &state = m_registry.get<DetonationStateComponent>(attacker_entity);
    if (state.has_detonated) {
        return;
    }
    const auto &route = m_registry.get<RouteComponent>(attacker_entity);
    double total_duration = route.total_duration;
    if (!std::isfinite(total_duration)) {
        return;
    }
    const auto &start = m_registry.get<StartSecComponent>(attacker_entity);
    if (static_cast<double>(time_sec) < static_cast<double>(start.value) + total_duration) {
        return;
    }

    const auto &pos = m_registry.get<PositionComponent>(attacker_entity).ecef;
    double lat = 0.0;
    double lon = 0.0;
    double alt = 0.0;
    ecefToGeodetic(pos, lat, lon, alt);
    jsonobj::DetonationEvent event;
    event.setEventType("detonation");
    event.setTimeSec(time_sec);
    event.setAttackerId(m_registry.get<ObjectIdComponent>(attacker_entity).value);
    event.setLatDeg(lat);
    event.setLonDeg(lon);
    event.setAltM(alt);
    event.setBomRangeM(m_bom_range_m);
    nlohmann::json json_event;
    jsonobj::to_json(json_event, event);
    m_event_logger.write(json_event);
    state.has_detonated = true;
}
