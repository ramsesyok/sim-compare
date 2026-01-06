#pragma once

#include <fstream>
#include <string>
#include <vector>

#include "json.hpp"

class SimObject;
class Simulation;

// タイムラインログの出力を専用クラスにまとめ、入出力の責務を独立させます。
class TimelineLogger {
public:
    void open(const std::string &path);
    void write(int time_sec, const std::vector<SimObject *> &objects, const Simulation &simulation);

private:
    std::ofstream m_out{};
};

// イベントログはシミュレーション全体で共有するため、単一インスタンスで管理します。
class EventLogger {
public:
    static EventLogger &instance();

    void open(const std::string &path);
    void write(const nlohmann::json &event);
    void close();

private:
    EventLogger() = default;
    EventLogger(const EventLogger &) = delete;
    EventLogger &operator=(const EventLogger &) = delete;

    std::ofstream m_out{};
};
