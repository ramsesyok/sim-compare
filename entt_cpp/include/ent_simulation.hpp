/**
 * @file ent_simulation.hpp
 * @brief EnTT版シミュレーションのクラス宣言をまとめたヘッダです。
 *
 * @details 初心者でも責務を追いやすいように、初期化・更新・イベント処理を1つのクラスに整理します。
 */
#pragma once

#include <string>
#include <vector>

#include "ecs_components.hpp"
#include "entt/entt.hpp"
#include "jsonobj/scenario.hpp"
#include "logging.hpp"
#include "spatial_hash.hpp"

/**
 * @brief シミュレーション全体の流れを管理するクラスです。
 *
 * @details ECS構成のレジストリをここで保持し、初期化と実行を分離することで、
 *          「準備する処理」と「繰り返し更新する処理」を初心者が区別しやすくします。
 */
class EnttSimulation
{
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
     * @brief シナリオからECSのレジストリを構築します。
     *
     * @details ここでエンティティとコンポーネントをそろえることで、run中の処理を単純化します。
     */
    void buildRegistry(const jsonobj::Scenario &scenario);
    /**
     * @brief 斥候1体分の探知・失探イベントを生成します。
     *
     * @details 空間ハッシュの近傍だけを調べ、イベント出力を最小限に抑えます。
     */

    /**
     * @brief 1体分の位置を計算し、ECEF座標として返します。
     *
     * @details 役割と開始時刻、ルート情報を入力として、補間結果だけを返す純粋計算です。
     */
    Ecef updatePositions(const RoleComponent &role,
                         const StartSecComponent &start,
                         const RouteComponent &route,
                         int time_sec);

    void updateDetections(
        int time_sec,
        entt::entity scout_entity,
        const std::unordered_map<CellKey, std::vector<entt::entity>, CellKeyHash> &spatial_hash);
    /**
     * @brief 攻撃役1体分の爆破イベントを生成します。
     *
     * @details 1回だけ発生させるため、内部の状態で再発火を抑制します。
     */
    void emitDetonations(int time_sec, entt::entity attacker_entity);

    bool m_initialized = false;
    jsonobj::Scenario m_scenario{};
    entt::registry m_registry{};
    std::vector<entt::entity> m_entities{};
    TimelineLogger m_timeline_logger{};
    EventLogger m_event_logger{};
    int m_end_sec = 24 * 60 * 60;
    int m_detect_range_m = 0;
    int m_comm_range_m = 0;
    int m_bom_range_m = 0;
};
