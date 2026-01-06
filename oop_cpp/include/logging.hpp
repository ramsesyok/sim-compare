#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"

class SimObject;
class Simulation;
namespace spdlog {
class logger;
}

/**
 * @brief タイムラインログの出力を専用クラスにまとめ、入出力の責務を独立させます。
 */
class TimelineLogger {
public:
    /**
     * @brief 出力先ファイルを開いてログ出力を開始します。
     */
    void open(const std::string &path);
    /**
     * @brief 1秒分のタイムライン情報を出力します。
     */
    void write(int time_sec, const std::vector<SimObject *> &objects, const Simulation &simulation);

private:
    /**
     * @brief spdlogのロガーを保持し、ファイル出力を統一します。
     */
    std::shared_ptr<spdlog::logger> m_logger{};
};

/**
 * @brief イベントログはシミュレーション全体で共有するため、単一インスタンスで管理します。
 */
class EventLogger {
public:
    /**
     * @brief シングルトンの唯一インスタンスを取得します。
     */
    static EventLogger &instance();

    /**
     * @brief 出力先ファイルを開いてイベントログ出力を開始します。
     */
    void open(const std::string &path);
    /**
     * @brief JSONイベントを1行のndjsonとして出力します。
     */
    void write(const nlohmann::json &event);
    /**
     * @brief 出力ファイルを明示的に閉じます。
     */
    void close();

private:
    EventLogger() = default;
    EventLogger(const EventLogger &) = delete;
    EventLogger &operator=(const EventLogger &) = delete;

    std::ofstream m_out{};
};
