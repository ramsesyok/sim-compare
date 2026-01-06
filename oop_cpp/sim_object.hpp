#pragma once

#include <string>
#include <vector>

#include "geo.hpp"
#include "route.hpp"
#include "scenario.hpp"

// シミュレーション内の共通状態を持つ基底クラスで、派生クラスが更新処理を担当します。
// オブジェクト指向としては「共通の性質と操作」を親クラスに集約し、役割ごとの差分を継承で分けます。
class SimObject {
public:
    SimObject(std::string id,
              std::string team_id,
              simoop::Role role,
              int start_sec,
              std::vector<RoutePoint> route,
              std::vector<std::string> network);
    virtual ~SimObject();

    // 位置更新は派生クラスに委ね、固定・移動などの違いを動的ディスパッチで吸収します。
    virtual void updatePosition(int time_sec) = 0;

    const std::string &id() const { return m_id; }
    const std::string &teamId() const { return m_team_id; }
    simoop::Role role() const { return m_role; }
    const Ecef &position() const { return m_position; }

protected:
    // m_で始まる変数はメンバ変数であることを明示し、ライフサイクルを見失わないようにします。
    std::string m_id;
    std::string m_team_id;
    simoop::Role m_role = simoop::Role::COMMANDER;
    int m_start_sec = 0;
    std::vector<RoutePoint> m_route;
    std::vector<std::string> m_network;
    Ecef m_position{};
};
