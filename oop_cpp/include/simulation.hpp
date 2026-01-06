#pragma once

#include <memory>
#include <string>
#include <vector>

#include "logging.hpp"
#include "jsonobj/scenario.hpp"
#include "sim_object.hpp"

/**
 * @brief シミュレーション全体の流れを管理するクラスです。
 *
 * @details 初期化と実行を明確に分けることで責務を整理します。
 */
class Simulation {
public:
    /**
     * @brief 役割を文字列に変換する処理は、ログ生成時の共通操作としてまとめています。
     */
    std::string roleToString(jsonobj::Role role) const;
    /**
     * @brief 初期化ではシナリオ読込と入出力の準備を行い、状態をクラスの内部に保持します。
     */
    void initialize(const std::string &scenario_path,
                    const std::string &timeline_path,
                    const std::string &event_path);
    /**
     * @brief runはループのみを担当し、initializeで準備された状態を使って実行します。
     */
    void run();

private:
    /**
     * @brief 具体的なオブジェクト生成は内部実装として隠蔽し、呼び出し側を単純にします。
     */
    std::vector<std::unique_ptr<SimObject>> buildObjects(const jsonobj::Scenario &scenario);
    /**
     * @brief シナリオを読み込んで内部構造へ変換します。
     */
    jsonobj::Scenario loadScenario(const std::string &path) const;

    /**
     * @brief 実行に必要な状態をメンバ変数として保持し、関数間で共有します。
     */
    bool m_initialized = false;
    jsonobj::Scenario m_scenario{};
    std::vector<std::unique_ptr<SimObject>> m_objects{};
    std::vector<SimObject *> m_object_ptrs{};
    TimelineLogger m_timeline_logger{};
    EventLogger m_event_logger{};
    int m_end_sec = 24 * 60 * 60;
    double m_detect_range = 0.0;
};
