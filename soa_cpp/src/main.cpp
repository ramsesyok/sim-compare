#include <iostream>
#include <string>

#include "CLI/CLI11.hpp"
#include "soa_simulation.hpp"

/**
 * @brief CLI引数の受け取り先をまとめる構造体です。
 */
struct Args {
    std::string scenario_path;
    std::string timeline_log_path;
    std::string event_log_path;
};

int main(int argc, char *argv[]) {
    CLI::App app{"SoA C++ simulation"};
    try {
        Args args;
        app.add_option("--scenario", args.scenario_path, "シナリオJSONのパス")
            ->required();
        app.add_option("--timeline-log", args.timeline_log_path, "タイムラインログの出力先")
            ->required();
        app.add_option("--event-log", args.event_log_path, "イベントログの出力先")
            ->required();

        app.parse(argc, argv);

        // mainは入出力の橋渡しだけを担当し、シミュレーション本体の責務はSimulationに委譲します。
        SoaSimulation simulation;
        simulation.initialize(args.scenario_path, args.timeline_log_path, args.event_log_path);
        simulation.run();
    } catch (const CLI::ParseError &error) {
        return app.exit(error);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
    return 0;
}
