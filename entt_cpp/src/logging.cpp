/**
 * @file logging.cpp
 * @brief タイムラインとイベントのログ出力を実装するファイルです。
 *
 * @details JSON化とファイル出力をここにまとめ、シミュレーション本体から分離します。
 */
#include "logging.hpp"
#include "ent_simulation.hpp"

#include <stdexcept>
#include <vector>

#include "spdlog/async.h"
#include "spdlog/sinks/basic_file_sink.h"

#include "ecs_components.hpp"
#include "geo.hpp"
#include "jsonobj/timeline.hpp"

/**
 * @brief タイムラインログの出力先を開きます。
 *
 * @details ファイルが開けない場合は例外で通知し、早期に失敗を検知します。
 */
void TimelineLogger::open(const std::string &path) {
    // タイムラインログの出力先を開き、失敗したら例外で知らせます。
    m_logger.reset();
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
    m_logger = std::make_shared<spdlog::logger>("entt_timeline_logger", sink);
    m_logger->set_level(spdlog::level::info);
    m_logger->set_pattern("%v");
    m_logger->flush_on(spdlog::level::info);
}

/**
 * @brief 1秒分のタイムライン情報を出力します。
 *
 * @details 位置や役割をJSONにまとめ、ndjson形式で1行ずつ書き込みます。
 */
void TimelineLogger::write(int time_sec,
                           const entt::registry &registry,
                           const std::vector<entt::entity> &entities,
                           const EnttSimulation &simulation) {
    // 1秒分のタイムラインをJSONにまとめ、ndjsonとして1行で出力します。
    if (!m_logger) {
        throw std::runtime_error("timeline: logger is not initialized");
    }

    jsonobj::Timeline timeline;
    timeline.setTimeSec(time_sec);
    std::vector<jsonobj::TimelinePosition> positions;
    positions.reserve(entities.size());

    for (entt::entity entity : entities) {
        const auto &object_id = registry.get<ObjectIdComponent>(entity);
        const auto &team_id = registry.get<TeamIdComponent>(entity);
        const auto &role = registry.get<RoleComponent>(entity);
        const auto &pos = registry.get<PositionComponent>(entity);
        jsonobj::TimelinePosition position;
        double lat = 0.0;
        double lon = 0.0;
        double alt = 0.0;
        ecefToGeodetic(
            pos.ecef,
            lat,
            lon,
            alt);
        position.setObjectId(object_id.value);
        position.setTeamId(team_id.value);
        position.setRole(simulation.roleToString(role.value));
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

/**
 * @brief イベントログの出力先を開きます。
 *
 * @details 非同期ロガーを初期化し、イベントが集中しても書き込みが詰まりにくくします。
 */
void EventLogger::open(const std::string &path) {
    // イベントログの出力先を開き、非同期ロガーを準備します。
    if (!spdlog::thread_pool()) {
        spdlog::init_thread_pool(8192, 1);
    }

    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path, true);
    m_logger = std::make_shared<spdlog::async_logger>(
        "entt_event_logger",
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

/**
 * @brief イベントJSONを1行で書き出します。
 *
 * @details ndjson形式で追記し、後段での解析を容易にします。
 */
void EventLogger::write(const nlohmann::json &event) {
    // イベントが発生したときに1行書き出すだけの関数です。
    if (!m_logger) {
        throw std::runtime_error("event: logger is not initialized");
    }
    m_logger->info("{}", event.dump());
}

/**
 * @brief ロガーを明示的に終了します。
 *
 * @details プログラム終了時にバッファを確実に出力するために呼び出します。
 */
void EventLogger::close() {
    // テストや終了処理で明示的に閉じるために用意しています。
    if (m_logger) {
        m_logger->flush();
        m_logger.reset();
    }
}
