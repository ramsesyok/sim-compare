#pragma once

#include <string>
#include <vector>

#include "geo.hpp"
#include "jsonobj/scenario.hpp"
#include "logging.hpp"
#include "aos_storage.hpp"
#include "spatial_hash.hpp"

/**
 * @brief シミュレーション全体の流れを管理するクラスです。
 *
 * @details AoS構成の配列をここで保持し、初期化と実行を分離することで、
 *          「準備する処理」と「繰り返し更新する処理」を初心者が区別しやすくします。
 */
class AosSimulation {
public:
    /**
     * @brief 役割を文字列に変換してログ出力時に利用します。
     *
     * @details 文字列化を一箇所に集約することで、ログ表記の揺れを防ぎます。
     */
    std::string roleToString(jsonobj::Role role) const;
    /**
     * @brief シナリオ読込とログ出力の準備を行います。
     *
     * @details ファイルを開く処理とデータ展開をここで済ませ、run中の処理を単純化します。
     */
    void initialize(const std::string &scenario_path,
                    const std::string &timeline_path,
                    const std::string &event_path);
    /**
     * @brief 24時間分の更新ループを実行します。
     *
     * @details 1秒刻みのループを回し、位置更新やイベント判定をここに集約します。
     */
    void run();

private:
    /**
     * @brief シナリオを読み込んで内部の構造に変換します。
     *
     * @details JSONの読み込みを隠蔽し、呼び出し側が入出力の詳細を意識しないで済むようにします。
     */
    jsonobj::Scenario loadScenario(const std::string &path) const;
    /**
     * @brief シナリオからSoA配列を生成します。
     *
     * @details ここで個体の初期状態をそろえることで、run中の処理を単純化します。
     */
    void buildStorage(const jsonobj::Scenario &scenario);
    /**
     * @brief 指定時刻に合わせて全オブジェクトの位置を更新します。
     *
     * @details ルート補間の手順を1箇所にまとめ、run内の責務を明確化します。
     */
    void updatePositions(int time_sec);
    /**
     * @brief 斥候1体分の探知・失探イベントを生成します。
     *
     * @details 空間ハッシュの近傍だけを調べ、イベント出力を最小限に抑えます。
     */
    void updateDetectionForScout(
        int time_sec,
        size_t scout_index,
        const std::unordered_map<CellKey, std::vector<int>, CellKeyHash> &spatial_hash);
    /**
     * @brief 攻撃役1体分の爆破イベントを生成します。
     *
     * @details 1回だけ発生させるため、内部の状態で再発火を抑制します。
     */
    void emitDetonationForAttacker(int time_sec, size_t attacker_index);

    bool m_initialized = false;
    jsonobj::Scenario m_scenario{};
    AosStorage m_storage{};
    TimelineLogger m_timeline_logger{};
    EventLogger m_event_logger{};
    int m_end_sec = 24 * 60 * 60;
    int m_detect_range_m = 0;
    int m_comm_range_m = 0;
    int m_bom_range_m = 0;
};
