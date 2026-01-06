#include "logging.hpp"

#include <iomanip>
#include <stdexcept>

#include "geo.hpp"
#include "sim_object.hpp"
#include "simulation.hpp"
#include "timeline.hpp"

void TimelineLogger::open(const std::string &path) {
    // タイムラインログの出力先を開き、失敗した場合は例外で通知します。
    m_out.open(path);
    if (!m_out) {
        throw std::runtime_error("timeline: failed to open " + path);
    }
    m_out << std::setprecision(10);
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
    m_out << json_timeline.dump() << '\n';
}

EventLogger &EventLogger::instance() {
    // シングルトンとして唯一のインスタンスを返し、全体でログ出力を共有します。
    static EventLogger logger;
    return logger;
}

void EventLogger::open(const std::string &path) {
    // イベントログの出力先を開き、出力フォーマットの設定もここで行います。
    if (m_out) {
        // 既に開いている場合は閉じてから再設定し、状態を明確にします。
        m_out.close();
    }
    m_out.open(path);
    if (!m_out) {
        throw std::runtime_error("event: failed to open " + path);
    }
    m_out << std::setprecision(10);
}

void EventLogger::write(const nlohmann::json &event) {
    // 出力ストリームが準備されている前提で1行ずつndjsonを書き出します。
    if (!m_out) {
        throw std::runtime_error("event: logger is not initialized");
    }
    m_out << event.dump() << '\n';
}

void EventLogger::close() {
    // テストや終了処理で明示的に閉じられるように用意します。
    if (m_out) {
        m_out.close();
    }
}
