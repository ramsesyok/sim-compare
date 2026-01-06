#include <iostream>
#include <stdexcept>
#include <string>

#include "CLI/CLI11.hpp"
#include "simulation.hpp"

/**
 * @brief CLI引数の受け取り先をまとめる構造体です。
 */
struct CliArgs {
    std::string scenario_path;
    std::string timeline_path;
    std::string event_path;
};

/**
 * @brief アプリケーションのエントリポイントです。
 */
int main(int argc, char **argv) {
    CLI::App app{"OOP C++ simulation"};
    try {
        CliArgs args;
        // CLI11で必須引数を定義し、手書きの解析ロジックを減らして読みやすくします。
        app.add_option("--scenario", args.scenario_path, "シナリオJSONのパス")
            ->required();
        app.add_option("--timeline-log", args.timeline_path, "タイムラインログの出力先")
            ->required();
        app.add_option("--event-log", args.event_path, "イベントログの出力先")
            ->required();
        app.parse(argc, argv);
        // mainは入出力の橋渡しだけを担当し、シミュレーション本体の責務はSimulationに委譲します。
        Simulation simulation;
        simulation.initialize(args.scenario_path, args.timeline_path, args.event_path);
        simulation.run();
    } catch (const CLI::ParseError &error) {
        return app.exit(error);
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << '\n';
        return 1;
    }
    return 0;
}
