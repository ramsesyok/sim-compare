#include "messenger_object.hpp"

#include <utility>

MessengerObject::MessengerObject(std::string id,
                                 std::string team_id,
                                 int start_sec,
                                 std::vector<RoutePoint> route,
                                 std::vector<std::string> network,
                                 std::vector<double> segment_end_secs,
                                 double total_duration_sec,
                                 int comm_range_m)
    : MovableObject(std::move(id),
                    std::move(team_id),
                    simoop::Role::MESSENGER,
                    start_sec,
                    std::move(route),
                    std::move(network),
                    std::move(segment_end_secs),
                    total_duration_sec),
      comm_range_m_(comm_range_m) {}
