#include "simulation.hpp"

#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "attacker_object.hpp"
#include "commander_object.hpp"
#include "logging.hpp"
#include "messenger_object.hpp"
#include "route.hpp"
#include "scout_object.hpp"
#include "sim_object.hpp"
#include "spatial_hash.hpp"

std::string Simulation::roleToString(simoop::Role role) const {
    // 役割の文字列化を一箇所にまとめ、ログ出力時の表現を統一します。
    switch (role) {
    case simoop::Role::COMMANDER:
        return "commander";
    case simoop::Role::SCOUT:
        return "scout";
    case simoop::Role::MESSENGER:
        return "messenger";
    case simoop::Role::ATTACKER:
        return "attacker";
    }
    return "unknown";
}

std::vector<std::unique_ptr<SimObject>> Simulation::buildObjects(const simoop::Scenario &scenario) const {
    // シナリオ定義を元に、役割に応じた派生クラスを生成します。
    std::vector<std::unique_ptr<SimObject>> objects;

    for (const auto &team : scenario.getTeams()) {
        for (const auto &obj : team.getObjects()) {
            // オブジェクト生成の前に経路と移動時間を計算しておき、更新処理で再利用します。
            std::vector<RoutePoint> route = buildRoute(obj.getRoute());
            auto segment_info = buildSegmentTimes(route);
            std::vector<double> segment_ends = std::move(segment_info.first);
            double total_duration = segment_info.second;
            std::vector<std::string> network;
            std::optional<std::vector<std::string>> network_opt = obj.getNetwork();
            if (network_opt) {
                network = *network_opt;
            }
            int start_sec = static_cast<int>(obj.getStartSec());

            switch (obj.getRole()) {
            case simoop::Role::COMMANDER:
                objects.push_back(std::make_unique<CommanderObject>(
                    obj.getId(), team.getId(), obj.getRole(), start_sec, route, network));
                break;
            case simoop::Role::SCOUT:
                objects.push_back(std::make_unique<ScoutObject>(
                    obj.getId(),
                    team.getId(),
                    start_sec,
                    route,
                    network,
                    segment_ends,
                    total_duration,
                    static_cast<int>(scenario.getPerformance().getScout().getDetectRangeM()),
                    static_cast<int>(scenario.getPerformance().getScout().getCommRangeM())));
                break;
            case simoop::Role::MESSENGER:
                objects.push_back(std::make_unique<MessengerObject>(
                    obj.getId(),
                    team.getId(),
                    start_sec,
                    route,
                    network,
                    segment_ends,
                    total_duration,
                    static_cast<int>(scenario.getPerformance().getMessenger().getCommRangeM())));
                break;
            case simoop::Role::ATTACKER:
                objects.push_back(std::make_unique<AttackerObject>(
                    obj.getId(),
                    team.getId(),
                    start_sec,
                    route,
                    network,
                    segment_ends,
                    total_duration,
                    static_cast<int>(scenario.getPerformance().getAttacker().getBomRangeM())));
                break;
            }
        }
    }

    return objects;
}

simoop::Scenario Simulation::loadScenario(const std::string &path) const {
    // シナリオ読み込みはSimulation内部で完結させ、外部に解析手順を露出しません。
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("scenario: failed to open " + path);
    }
    nlohmann::json data;
    file >> data;
    simoop::Scenario scenario;
    simoop::from_json(data, scenario);
    return scenario;
}

void Simulation::initialize(const std::string &scenario_path,
                            const std::string &timeline_path,
                            const std::string &event_path) {
    // ここではAoS/SoA/ECSではなく、各オブジェクトをクラスとして扱うオブジェクト指向設計で、
    // 毎秒の更新やイベント判定をそれぞれの責務として分けて実装する流れを示しています。
    // initializeは準備だけを行い、runでは繰り返し処理のみを担当します。
    m_scenario = loadScenario(scenario_path);
    m_objects = buildObjects(m_scenario);

    m_object_ptrs.clear();
    m_object_ptrs.reserve(m_objects.size());
    for (const auto &obj : m_objects) {
        m_object_ptrs.push_back(obj.get());
    }

    m_timeline_logger.open(timeline_path);
    m_event_logger.open(event_path);

    m_end_sec = 24 * 60 * 60;
    m_detect_range = static_cast<double>(m_scenario.getPerformance().getScout().getDetectRangeM());
    m_initialized = true;
}

void Simulation::run() {
    if (!m_initialized) {
        throw std::runtime_error("simulation: initialize must be called before run");
    }

    // 1秒刻みで、位置更新→探知→爆破→ログ出力の順に処理します。
    for (int time_sec = 0; time_sec <= m_end_sec; ++time_sec) {
        for (auto &obj : m_objects) {
            obj->updatePosition(time_sec);
        }

        std::unordered_map<CellKey, std::vector<int>, CellKeyHash> spatial_hash =
            buildSpatialHash(m_object_ptrs, m_detect_range);

        for (size_t i = 0; i < m_objects.size(); ++i) {
            auto *scout = dynamic_cast<ScoutObject *>(m_objects[i].get());
            if (scout) {
                scout->updateDetection(time_sec, spatial_hash, m_object_ptrs, static_cast<int>(i), m_event_logger);
            }
        }

        for (auto &obj : m_objects) {
            auto *attacker = dynamic_cast<AttackerObject *>(obj.get());
            if (attacker) {
                attacker->emitDetonation(time_sec, m_event_logger);
            }
        }

        m_timeline_logger.write(time_sec, m_object_ptrs, *this);
    }
}
