/**
 * @file ent_simulation.cpp
 * @brief EnTT版シミュレーション本体の処理をまとめた実装ファイルです。
 *
 * @details 初心者が処理の流れを追いやすいように、初期化・更新・イベント発火を
 *          1つのクラスに集約しています。System専用の型は使わず、関数として実装します。
 */
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

/**
 * @brief 役割の列挙値をログ用の文字列へ変換します。
 *
 * @details ログ出力で役割名が統一されるように、変換処理を1箇所に固定します。
 */
std::string EnttSimulation::roleToString(jsonobj::Role role) const
{
    // 役割の文字列化を一箇所にまとめて、ログ出力の表記を統一します。
    switch (role)
    {
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

/**
 * @brief シナリオJSONを読み込み、内部構造に変換します。
 *
 * @details 読み込みの失敗は例外として通知し、呼び出し側が異常を検知できるようにします。
 */
jsonobj::Scenario EnttSimulation::loadScenario(const std::string &path) const
{
    // シナリオ読み込みはEnttSimulation内部で完結させ、外部に解析手順を露出しません。
    std::ifstream file(path);
    if (!file)
    {
        throw std::runtime_error("scenario: failed to open " + path);
    }
    nlohmann::json data;
    file >> data;
    jsonobj::Scenario scenario;
    jsonobj::from_json(data, scenario);
    return scenario;
}

/**
 * @brief シナリオ内容からレジストリを構築します。
 *
 * @details エンティティ生成とコンポーネント付与をまとめて行い、更新ループは単純化します。
 */
void EnttSimulation::buildRegistry(const jsonobj::Scenario &scenario)
{
    // ECSではエンティティに必要なコンポーネントだけを付与します。
    // ここでレジストリを組み立てておくと、run中の処理が単純になります。
    m_registry.clear();
    m_entities.clear();

    for (const auto &team : scenario.getTeams())
    {
        for (const auto &obj : team.getObjects())
        {
            std::vector<RoutePoint> route = buildRoute(obj.getRoute());
            auto segment_info = buildSegmentTimes(route);

            RouteComponent route_component;
            route_component.points = std::move(route);
            route_component.segment_end_secs = std::move(segment_info.first);
            route_component.total_duration = segment_info.second;

            Ecef start_ecef{0.0, 0.0, 0.0};
            if (!route_component.points.empty())
            {
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

            if (obj.getRole() == jsonobj::Role::SCOUT)
            {
                m_registry.emplace<DetectionRangeComponent>(entity, m_detect_range_m);
                m_registry.emplace<DetectionStateComponent>(entity);
            }
            if (obj.getRole() == jsonobj::Role::ATTACKER)
            {
                m_registry.emplace<DetonationRangeComponent>(entity, m_bom_range_m);
                m_registry.emplace<DetonationStateComponent>(entity);
            }
        }
    }
}

/**
 * @brief シミュレーション実行に必要な前準備を行います。
 *
 * @details ログ出力先の初期化とシナリオ読み込みを先に済ませ、
 *          runが毎秒の更新だけに集中できるようにします。
 */
void EnttSimulation::initialize(const std::string &scenario_path,
                                const std::string &timeline_path,
                                const std::string &event_path)
{
    // ECS(EnTT)では「エンティティに必要なコンポーネントだけを付ける」ことで、
    // 処理対象を絞り込みやすくします。ここではシナリオからエンティティを生成し、
    // runでは「位置更新」「探知」「爆破」などの処理を役割ごとに分けて実行します。
    // initializeは準備だけに集中し、runは毎秒の更新ループに専念させます。
    m_event_logger.open(event_path);
    m_timeline_logger.open(timeline_path);
    m_scenario = loadScenario(scenario_path);
    m_end_sec = 24 * 60 * 60;
    m_detect_range_m = static_cast<int>(m_scenario.getPerformance().getScout().getDetectRangeM());
    m_comm_range_m = static_cast<int>(m_scenario.getPerformance().getScout().getCommRangeM());
    m_bom_range_m = static_cast<int>(m_scenario.getPerformance().getAttacker().getBomRangeM());
    buildRegistry(m_scenario);
    m_initialized = true;
}

/**
 * @brief 1秒ごとの更新ループを実行します。
 *
 * @details 位置更新・探知・爆破を順番に呼び出し、時間の流れを明確にします。
 */
void EnttSimulation::run()
{
    if (!m_initialized)
    {
        throw std::runtime_error("simulation: initialize must be called before run");
    }

    // ECSでは「システム=処理のまとまり」を関数として実装し、
    // 1秒ごとの更新も「位置更新」「探知」「爆破」の順で明示的に呼び出します。
    // EnTTはSystem専用の型を用意しないため、ここではレジストリに対する処理を
    // 手続き的に並べて「更新の流れ」を見える化しています。
    for (int time_sec = 0; time_sec <= m_end_sec; ++time_sec)
    {
        // 位置更新は「Position/Route/Start/Roleを持つエンティティだけ」に適用します。
        // viewは該当コンポーネントを持つ集合だけを返すので、不要な分岐を減らせます。
        // ECSでは「必要なデータを持つものだけを対象にする」のが基本です。
        auto view = m_registry.view<RoleComponent,
                                    StartSecComponent,
                                    RouteComponent,
                                    PositionComponent>();
        view.each([&](const RoleComponent &role,
                      const StartSecComponent &start,
                      const RouteComponent &route,
                      PositionComponent &pos)
                  {
                      // 位置計算は純粋計算として切り出し、副作用の場所を明確にします。
                      pos.ecef = updatePositions(role, start, route, time_sec);
                  });

        // 探知処理は近傍探索が重いので、空間ハッシュで候補を絞ります。
        // ここではセルサイズを「シナリオ共通の探知距離」に合わせています。
        // 各エンティティごとの距離判定は後段のupdateDetectionsで行います。
        std::unordered_map<CellKey, std::vector<entt::entity>, CellKeyHash> spatial_hash =
            buildSpatialHash(m_registry, m_entities, static_cast<double>(m_detect_range_m));

        // DetectionRangeComponentを持つエンティティだけを対象にします。
        // ECSでは「役割の分岐」よりも「コンポーネントの有無」で対象を決めます。
        for (entt::entity entity : m_entities)
        {
            if (m_registry.all_of<DetectionRangeComponent>(entity))
            {
                updateDetections(time_sec, entity, spatial_hash);
            }
        }

        // DetonationRangeComponentを持つエンティティだけが爆破処理を担当します。
        // これにより、役割分岐のifを減らし、データ駆動で処理対象を選びます。
        for (entt::entity entity : m_entities)
        {
            if (m_registry.all_of<DetonationRangeComponent>(entity))
            {
                emitDetonations(time_sec, entity);
            }
        }
        // タイムラインは1秒ごとの結果を丸ごと出力します。
        // 出力のタイミングを統一することで、ログの時系列が揃います。
        m_timeline_logger.write(time_sec, m_registry, m_entities, *this);
    }
}

/**
 * @brief 斥候1体分の探知イベントを更新します。
 *
 * @details 空間ハッシュで候補を絞り、距離計算とFOUND/LOSTの判定を行います。
 */
void EnttSimulation::updateDetections(
    int time_sec,
    entt::entity scout_entity,
    const std::unordered_map<CellKey, std::vector<entt::entity>, CellKeyHash> &spatial_hash)
{
    // 探知範囲が無効なら処理を省略し、無駄な計算を避けます。
    const auto &range = m_registry.get<DetectionRangeComponent>(scout_entity);
    if (range.range_m <= 0)
    {
        return;
    }

    std::unordered_map<std::string, DetectionInfo> current_detected;
    const auto &scout_pos = m_registry.get<PositionComponent>(scout_entity).ecef;
    const auto &scout_team = m_registry.get<TeamIdComponent>(scout_entity).value;
    const auto &scout_id = m_registry.get<ObjectIdComponent>(scout_entity).value;
    CellKey base = cellKey(scout_pos, static_cast<double>(range.range_m));

    for (int dx = -1; dx <= 1; ++dx)
    {
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dz = -1; dz <= 1; ++dz)
            {
                CellKey key{base.x + dx, base.y + dy, base.z + dz};
                auto it = spatial_hash.find(key);
                if (it == spatial_hash.end())
                {
                    continue;
                }
                for (entt::entity other_entity : it->second)
                {
                    if (other_entity == scout_entity)
                    {
                        continue;
                    }
                    const auto &other_team = m_registry.get<TeamIdComponent>(other_entity).value;
                    if (other_team == scout_team)
                    {
                        continue;
                    }
                    const auto &other_pos = m_registry.get<PositionComponent>(other_entity).ecef;
                    double distance = distanceEcef(scout_pos, other_pos);
                    if (distance > static_cast<double>(range.range_m))
                    {
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
    for (const auto &entry : current_detected)
    {
        if (previous_detected.find(entry.first) != previous_detected.end())
        {
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

    for (const auto &entry : previous_detected)
    {
        if (current_detected.find(entry.first) != current_detected.end())
        {
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

/**
 * @brief 攻撃役1体分の爆破イベントを発火します。
 *
 * @details 既に発火済みかどうかを確認し、1回だけ出力するように制御します。
 */
void EnttSimulation::emitDetonations(int time_sec, entt::entity attacker_entity)
{
    // 爆破イベントは攻撃役の責務として扱い、1回だけ発火させます。
    auto &state = m_registry.get<DetonationStateComponent>(attacker_entity);
    if (state.has_detonated)
    {
        return;
    }
    const auto &range = m_registry.get<DetonationRangeComponent>(attacker_entity);
    if (range.range_m <= 0)
    {
        return;
    }
    const auto &route = m_registry.get<RouteComponent>(attacker_entity);
    double total_duration = route.total_duration;
    if (!std::isfinite(total_duration))
    {
        return;
    }
    const auto &start = m_registry.get<StartSecComponent>(attacker_entity);
    if (static_cast<double>(time_sec) < static_cast<double>(start.value) + total_duration)
    {
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
    event.setBomRangeM(range.range_m);
    nlohmann::json json_event;
    jsonobj::to_json(json_event, event);
    m_event_logger.write(json_event);
    state.has_detonated = true;
}

/**
 * @brief 1体分の位置を計算し、ECEF座標として返します。
 *
 * @details 役割と開始時刻、ルート情報から補間位置を求める純粋計算です。
 */
Ecef EnttSimulation::updatePositions(const RoleComponent &role,
                                     const StartSecComponent &start,
                                     const RouteComponent &route,
                                     int time_sec)
{
    // 位置更新の計算だけを純粋関数として切り出し、副作用を持たない形にします。
    // 入力が同じなら必ず同じ結果になるため、更新ループの理解が容易になります。
    size_t route_count = route.points.size();
    if (route_count == 0)
    {
        return Ecef{0.0, 0.0, 0.0};
    }

    const RoutePoint &first = route.points.front();

    if (role.value == jsonobj::Role::COMMANDER)
    {
        return first.ecef;
    }

    if (time_sec < start.value)
    {
        return first.ecef;
    }

    size_t segment_count = route.segment_end_secs.size();
    if (segment_count == 0)
    {
        const RoutePoint &last = route.points.back();
        return last.ecef;
    }

    double elapsed = static_cast<double>(time_sec - start.value);
    double total_duration = route.total_duration;
    if (elapsed >= total_duration)
    {
        const RoutePoint &last = route.points.back();
        return last.ecef;
    }

    auto it = std::upper_bound(route.segment_end_secs.begin(),
                               route.segment_end_secs.end(),
                               elapsed);
    size_t segment_index = static_cast<size_t>(std::distance(route.segment_end_secs.begin(), it));

    if (segment_index >= segment_count)
    {
        const RoutePoint &last = route.points.back();
        return last.ecef;
    }

    double segment_end = route.segment_end_secs[segment_index];
    double segment_start = (segment_index == 0)
                               ? 0.0
                               : route.segment_end_secs[segment_index - 1];
    double segment_duration = segment_end - segment_start;
    if (segment_duration <= 0.0)
    {
        const RoutePoint &target = route.points[segment_index + 1];
        return target.ecef;
    }
    if (!std::isfinite(segment_duration))
    {
        const RoutePoint &target = route.points[segment_index];
        return target.ecef;
    }

    double t = (elapsed - segment_start) / segment_duration;
    const RoutePoint &a = route.points[segment_index];
    const RoutePoint &b = route.points[segment_index + 1];
    return Ecef{
        a.ecef.x + (b.ecef.x - a.ecef.x) * t,
        a.ecef.y + (b.ecef.y - a.ecef.y) * t,
        a.ecef.z + (b.ecef.z - a.ecef.z) * t,
    };
}
