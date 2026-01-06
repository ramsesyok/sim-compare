#pragma once

#include <string>
#include <vector>

#include "geo.hpp"
#include "route.hpp"
#include "jsonobj/scenario.hpp"

/**
 * @brief シミュレーション内の共通状態を持つ基底クラスです。
 *
 * @details オブジェクト指向としては「共通の性質と操作」を親クラスに集約し、
 *          役割ごとの差分を継承で分けることで責務を整理します。
 */
class SimObject {
public:
    /**
     * @brief 共通データを受け取り、派生クラスで再利用できるように初期化します。
     */
    SimObject(std::string id,
              std::string team_id,
              jsonobj::Role role,
              int start_sec,
              std::vector<RoutePoint> route,
              std::vector<std::string> network);
    /**
     * @brief 継承階層を安全に破棄するための仮想デストラクタです。
     */
    virtual ~SimObject();

    /**
     * @brief 位置更新は派生クラスに委ね、固定・移動などの違いを動的ディスパッチで吸収します。
     */
    virtual void updatePosition(int time_sec) = 0;

    /**
     * @brief オブジェクト識別子を返します。
     */
    const std::string &id() const { return m_id; }
    /**
     * @brief チーム識別子を返します。
     */
    const std::string &teamId() const { return m_team_id; }
    /**
     * @brief 役割を返します。
     */
    jsonobj::Role role() const { return m_role; }
    /**
     * @brief 現在位置を返します。
     */
    const Ecef &position() const { return m_position; }

protected:
    /**
     * @brief m_で始まる変数はメンバ変数であることを明示し、ライフサイクルを見失わないようにします。
     */
    std::string m_id;
    std::string m_team_id;
    jsonobj::Role m_role = jsonobj::Role::COMMANDER;
    int m_start_sec = 0;
    std::vector<RoutePoint> m_route;
    std::vector<std::string> m_network;
    Ecef m_position{};
};
