#include "simulation.hpp"

#include <fstream>
#include <iomanip>
#include <optional>
#include <stdexcept>
#include <unordered_map>
#include <utility>

#include "attacker_object.hpp"
#include "commander_object.hpp"
#include "loggin.hpp"
#include "messenger_object.hpp"
#include "route.hpp"
#include "scout_object.hpp"
#include "sim_object.hpp"
#include "spatial_hash.hpp"

std::string Simulation::roleToString(simoop::Role role) const {
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
    std::vector<std::unique_ptr<SimObject>> objects;

    for (const auto &team : scenario.getTeams()) {
        for (const auto &obj : team.getObjects()) {
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
    scenario_ = loadScenario(scenario_path);
    objects_ = buildObjects(scenario_);

    object_ptrs_.clear();
    object_ptrs_.reserve(objects_.size());
    for (const auto &obj : objects_) {
        object_ptrs_.push_back(obj.get());
    }

    timeline_out_.open(timeline_path);
    if (!timeline_out_) {
        throw std::runtime_error("timeline: failed to open " + timeline_path);
    }
    event_out_.open(event_path);
    if (!event_out_) {
        throw std::runtime_error("event: failed to open " + event_path);
    }

    timeline_out_ << std::setprecision(10);
    event_out_ << std::setprecision(10);

    end_sec_ = 24 * 60 * 60;
    detect_range_ = static_cast<double>(scenario_.getPerformance().getScout().getDetectRangeM());
    initialized_ = true;
}

void Simulation::run() {
    if (!initialized_) {
        throw std::runtime_error("simulation: initialize must be called before run");
    }

    for (int time_sec = 0; time_sec <= end_sec_; ++time_sec) {
        for (auto &obj : objects_) {
            obj->updatePosition(time_sec);
        }

        std::unordered_map<CellKey, std::vector<int>, CellKeyHash> spatial_hash =
            buildSpatialHash(object_ptrs_, detect_range_);

        for (size_t i = 0; i < objects_.size(); ++i) {
            auto *scout = dynamic_cast<ScoutObject *>(objects_[i].get());
            if (scout) {
                scout->updateDetection(time_sec, spatial_hash, object_ptrs_, static_cast<int>(i), event_out_);
            }
        }

        for (auto &obj : objects_) {
            auto *attacker = dynamic_cast<AttackerObject *>(obj.get());
            if (attacker) {
                attacker->emitDetonation(time_sec, event_out_);
            }
        }

        writeTimelineLog(time_sec, object_ptrs_, *this, timeline_out_);
    }
}
