/**
 * @file main.cpp
 * @brief EnTT版シミュレーションのエントリポイントです。
 *
 * @details CLI引数の解析と実行開始だけを担当し、処理本体はEnttSimulationに委譲します。
 */
#include <iostream>
#include <string>

#include "CLI/CLI11.hpp"
#include "ent_simulation.hpp"

/**
 * @brief CLI引数の受け取り先をまとめる構造体です。
 */
struct Args {
    std::string scenario_path;
    std::string timeline_log_path;
    std::string event_log_path;
};

/**
 * @brief CLI引数を受け取り、シミュレーションを起動します。
 *
 * @details 例外処理を行い、入力ミスや実行エラーを利用者に伝えます。
 */
int main(int argc, char *argv[]) {
    // ECS(EnTT)ではエンティティとコンポーネントを分離して管理し、役割ごとに処理を分けます。
    // mainは入出力の入口として最小限の責務だけを持ち、実際の更新はEnttSimulationに委譲します。
    CLI::App app{"EnTT C++ simulation"};
    try {
        Args args;
        app.add_option("--scenario", args.scenario_path, "シナリオJSONのパス")
            ->required();
        app.add_option("--timeline-log", args.timeline_log_path, "タイムラインログの出力先")
            ->required();
        app.add_option("--event-log", args.event_log_path, "イベントログの出力先")
            ->required();

        app.parse(argc, argv);

        // mainは入出力の橋渡しだけを担当し、シミュレーション本体の責務はEnttSimulationに委譲します。
        EnttSimulation simulation;
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
