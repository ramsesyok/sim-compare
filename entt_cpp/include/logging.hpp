#pragma once

#include <memory>
#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "spdlog/spdlog.h"

#include "entt/entt.hpp"

class EnttSimulation;

/**
 * @brief 1秒ごとの位置情報をまとめたタイムラインログを出力するためのクラスです。
 *
 * @details シミュレーション本体から「何を書き出すか」を分離し、ファイル操作やJSON変換を
 *          ここに集約することで、初心者でも責務の境界を理解しやすくします。
 */
class TimelineLogger {
public:
    /**
     * @brief 出力先ファイルを開いてロガーを初期化します。
     *
     * @details ログ書き出しに必要な準備をここで行い、run中は書き出しだけに集中できるようにします。
     */
    void open(const std::string &path);
    /**
     * @brief 1秒分のタイムラインログを生成して書き出します。
     *
     * @details ECSのコンポーネントから必要な情報を抜き出し、JSONへまとめて1行で保存します。
     */
    void write(int time_sec,
               const entt::registry &registry,
               const std::vector<entt::entity> &entities,
               const EnttSimulation &simulation);

private:
    std::shared_ptr<spdlog::logger> m_logger{};
};

/**
 * @brief 探知や爆破などのイベントログを出力するためのクラスです。
 *
 * @details イベントは発生した時点で1行ずつ書き出す想定なので、タイムラインとは別クラスにします。
 */
class EventLogger {
public:
    /**
     * @brief 出力先ファイルを開いてロガーを初期化します。
     *
     * @details 非同期ロガーを利用して、イベント発生が集中しても出力待ちが起きにくい構成にします。
     */
    void open(const std::string &path);
    /**
     * @brief JSON形式のイベントを1行で書き出します。
     *
     * @details ndjson形式で追記することで、後処理を単純にできるようにしています。
     */
    void write(const nlohmann::json &event);
    /**
     * @brief 終了時に明示的にクローズします。
     *
     * @details テストや終了処理でファイルを閉じたいときに、ここを呼び出せるようにします。
     */
    void close();

private:
    std::shared_ptr<spdlog::logger> m_logger{};
};
