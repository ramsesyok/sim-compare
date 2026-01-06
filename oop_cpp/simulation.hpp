#pragma once

#include <fstream>
#include <memory>
#include <string>
#include <vector>

#include "scenario.hpp"

class SimObject;

class Simulation {
public:
    std::string roleToString(simoop::Role role) const;
    void initialize(const std::string &scenario_path,
                    const std::string &timeline_path,
                    const std::string &event_path);
    void run();

private:
    std::vector<std::unique_ptr<SimObject>> buildObjects(const simoop::Scenario &scenario) const;
    simoop::Scenario loadScenario(const std::string &path) const;

    bool initialized_ = false;
    simoop::Scenario scenario_{};
    std::vector<std::unique_ptr<SimObject>> objects_{};
    std::vector<SimObject *> object_ptrs_{};
    std::ofstream timeline_out_{};
    std::ofstream event_out_{};
    int end_sec_ = 24 * 60 * 60;
    double detect_range_ = 0.0;
};
