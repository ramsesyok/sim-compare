#include <iostream>
#include <stdexcept>
#include <string>

#include "simulation.hpp"

struct CliArgs {
    std::string scenario_path;
    std::string timeline_path;
    std::string event_path;
};

static CliArgs parseArgs(int argc, char **argv) {
    CliArgs args;
    for (int i = 1; i < argc; ++i) {
        std::string token = argv[i];
        if (token == "--scenario" && i + 1 < argc) {
            args.scenario_path = argv[++i];
        } else if (token == "--timeline-log" && i + 1 < argc) {
            args.timeline_path = argv[++i];
        } else if (token == "--event-log" && i + 1 < argc) {
            args.event_path = argv[++i];
        } else {
            throw std::runtime_error("usage: --scenario <path> --timeline-log <path> --event-log <path>");
        }
    }
    if (args.scenario_path.empty() || args.timeline_path.empty() || args.event_path.empty()) {
        throw std::runtime_error("usage: --scenario <path> --timeline-log <path> --event-log <path>");
    }
    return args;
}

int main(int argc, char **argv) {
    try {
        CliArgs args = parseArgs(argc, argv);
        // mainは入出力の橋渡しだけを担当し、シミュレーション本体の責務はSimulationに委譲します。
        Simulation simulation;
        simulation.initialize(args.scenario_path, args.timeline_path, args.event_path);
        simulation.run();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
    return 0;
}
