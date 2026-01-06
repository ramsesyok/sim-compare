#pragma once

#include <string>
#include <vector>

#include "geo.hpp"
#include "route.hpp"
#include "scenario.hpp"

// シミュレーション内の共通状態を持つ基底クラスで、派生クラスが更新処理を担当します。
class SimObject {
public:
    SimObject(std::string id,
              std::string team_id,
              simoop::Role role,
              int start_sec,
              std::vector<RoutePoint> route,
              std::vector<std::string> network);
    virtual ~SimObject();

    virtual void updatePosition(int time_sec) = 0;

    const std::string &id() const { return id_; }
    const std::string &teamId() const { return team_id_; }
    simoop::Role role() const { return role_; }
    const Ecef &position() const { return position_; }

protected:
    std::string id_;
    std::string team_id_;
    simoop::Role role_ = simoop::Role::COMMANDER;
    int start_sec_ = 0;
    std::vector<RoutePoint> route_;
    std::vector<std::string> network_;
    Ecef position_{};
};
