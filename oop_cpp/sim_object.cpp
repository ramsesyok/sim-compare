#include "sim_object.hpp"

#include <utility>

SimObject::SimObject(std::string id,
                     std::string team_id,
                     simoop::Role role,
                     int start_sec,
                     std::vector<RoutePoint> route,
                     std::vector<std::string> network)
    : id_(std::move(id)),
      team_id_(std::move(team_id)),
      role_(role),
      start_sec_(start_sec),
      route_(std::move(route)),
      network_(std::move(network)) {
    // 初期位置は経路の先頭に合わせて、移動前でも座標が決まるようにします。
    if (!route_.empty()) {
        position_ = route_.front().ecef;
    }
}

SimObject::~SimObject() = default;
