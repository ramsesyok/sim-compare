#include "logging.hpp"

#include <stdexcept>
#include <vector>

#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "geo.hpp"
#include "jsonobj/timeline.hpp"
#include "soa_storage.hpp"
#include "simulation.hpp"

void TimelineLogger::open(const std::string &path) {
    // タイムラインログの出力先を開き、失敗したら例外で知らせます。
    m_logger.reset();
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
    m_logger = std::make_shared<spdlog::logger>("soa_timeline_logger", sink);
    m_logger->set_level(spdlog::level::info);
    m_logger->set_pattern("%v");
    m_logger->flush_on(spdlog::level::info);
}

void TimelineLogger::write(int time_sec, const SoaStorage &storage, const Simulation &simulation) {
    // 1秒分のタイムラインをJSONにまとめ、ndjsonとして1行で出力します。
    if (!m_logger) {
        throw std::runtime_error("timeline: logger is not initialized");
    }

    jsonobj::Timeline timeline;
    timeline.setTimeSec(time_sec);
    std::vector<jsonobj::TimelinePosition> positions;
    positions.reserve(storage.object_ids.size());

    for (size_t i = 0; i < storage.object_ids.size(); ++i) {
        jsonobj::TimelinePosition position;
        double lat = 0.0;
        double lon = 0.0;
        double alt = 0.0;
        ecefToGeodetic(
            Ecef{storage.ecef_xs[i], storage.ecef_ys[i], storage.ecef_zs[i]},
            lat,
            lon,
            alt);
        position.setObjectId(storage.object_ids[i]);
        position.setTeamId(storage.team_ids[i]);
        position.setRole(simulation.roleToString(storage.roles[i]));
        position.setLatDeg(lat);
        position.setLonDeg(lon);
        position.setAltM(alt);
        positions.push_back(position);
    }

    timeline.setPositions(positions);
    nlohmann::json json_timeline;
    jsonobj::to_json(json_timeline, timeline);
    m_logger->info("{}", json_timeline.dump());
}

void EventLogger::open(const std::string &path) {
    // イベントログの出力先を開き、非同期ロガーを準備します。
    if (!spdlog::thread_pool()) {
        spdlog::init_thread_pool(8192, 1);
    }

    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
    m_logger = std::make_shared<spdlog::async_logger>(
        "soa_event_logger",
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
    // イベントが発生したときに1行書き出すだけの関数です。
    if (!m_logger) {
        throw std::runtime_error("event: logger is not initialized");
    }
    m_logger->info("{}", event.dump());
}

void EventLogger::close() {
    // テストや終了処理で明示的に閉じるために用意しています。
    if (m_logger) {
        m_logger->flush();
        m_logger.reset();
    }
}
