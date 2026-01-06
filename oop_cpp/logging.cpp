#include "logging.hpp"

#include <stdexcept>

#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "spdlog/spdlog.h"

#include "geo.hpp"
#include "sim_object.hpp"
#include "simulation.hpp"
#include "timeline.hpp"

void TimelineLogger::open(const std::string &path) {
    // タイムラインログの出力先を開き、失敗した場合は例外で通知します。
    m_logger.reset();
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
    m_logger = std::make_shared<spdlog::logger>("timeline_logger", sink);
    m_logger->set_level(spdlog::level::info);
    m_logger->set_pattern("%v");
    m_logger->flush_on(spdlog::level::info);
}

void TimelineLogger::write(int time_sec, const std::vector<SimObject *> &objects, const Simulation &simulation) {
    // タイムラインログは1秒ごとの全オブジェクト位置をまとめて出力します。
    // ログ出力を独立した関数にすることで、シミュレーションの責務を分割します。
    simoop::Timeline timeline;
    timeline.setTimeSec(time_sec);
    std::vector<simoop::TimelinePosition> positions;
    positions.reserve(objects.size());
    for (size_t i = 0; i < objects.size(); ++i) {
        const SimObject *obj = objects[i];
        double lat = 0.0;
        double lon = 0.0;
        double alt = 0.0;
        ecefToGeodetic(obj->position(), lat, lon, alt);
        simoop::TimelinePosition position;
        position.setObjectId(obj->id());
        position.setTeamId(obj->teamId());
        position.setRole(simulation.roleToString(obj->role()));
        position.setLatDeg(lat);
        position.setLonDeg(lon);
        position.setAltM(alt);
        positions.push_back(position);
    }
    timeline.setPositions(positions);
    nlohmann::json json_timeline;
    simoop::to_json(json_timeline, timeline);
    if (!m_logger) {
        throw std::runtime_error("timeline: logger is not initialized");
    }
    m_logger->info("{}", json_timeline.dump());
}

void EventLogger::open(const std::string &path) {
    // イベントログの出力先を開き、非同期ロガーの初期化もここで行います。
    if (!spdlog::thread_pool()) {
        spdlog::init_thread_pool(8192, 1);
    }

    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
    m_logger = std::make_shared<spdlog::async_logger>(
        "event_logger",
        sink,
        spdlog::thread_pool(),
        spdlog::async_overflow_policy::block);

    m_logger->set_level(spdlog::level::info);
    m_logger->set_pattern("%v");
    m_logger->flush_on(spdlog::level::info);

    if (!m_logger) {
        throw std::runtime_error("event: failed to open " + path);
    }
}

void EventLogger::write(const nlohmann::json &event) {
    // 出力ロガーが準備されている前提で1行ずつndjsonを書き出します。
    if (!m_logger) {
        throw std::runtime_error("event: logger is not initialized");
    }
    m_logger->info("{}", event.dump());
}

void EventLogger::close() {
    // テストや終了処理で明示的に閉じられるように用意します。
    if (m_logger) {
        m_logger->flush();
        m_logger.reset();
    }
}
