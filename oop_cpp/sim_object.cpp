#include "sim_object.hpp"

#include <utility>

SimObject::SimObject(std::string id,
                     std::string team_id,
                     simoop::Role role,
                     int start_sec,
                     std::vector<RoutePoint> route,
                     std::vector<std::string> network)
    : m_id(std::move(id)),
      m_team_id(std::move(team_id)),
      m_role(role),
      m_start_sec(start_sec),
      m_route(std::move(route)),
      m_network(std::move(network)) {
    // 初期位置は経路の先頭に合わせて、移動前でも座標が決まるようにします。
    if (!m_route.empty()) {
        m_position = m_route.front().ecef;
    }
}

SimObject::~SimObject() = default;
