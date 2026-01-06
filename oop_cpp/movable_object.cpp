#include "movable_object.hpp"

#include <algorithm>
#include <cmath>
#include <utility>

MovableObject::MovableObject(std::string id,
                             std::string team_id,
                             simoop::Role role,
                             int start_sec,
                             std::vector<RoutePoint> route,
                             std::vector<std::string> network,
                             std::vector<double> segment_end_secs,
                             double total_duration_sec)
    : SimObject(std::move(id), std::move(team_id), role, start_sec, std::move(route), std::move(network)),
      segment_end_secs_(std::move(segment_end_secs)),
      total_duration_sec_(total_duration_sec) {}

void MovableObject::updatePosition(int time_sec) {
    // 司令官は移動しないため、経路の先頭に固定します。
    if (role_ == simoop::Role::COMMANDER) {
        if (!route_.empty()) {
            position_ = route_.front().ecef;
        }
        return;
    }

    if (route_.empty()) {
        position_ = Ecef{};
        return;
    }

    // 移動開始時間前は最初の経路点に待機します。
    if (time_sec < start_sec_) {
        position_ = route_.front().ecef;
        return;
    }

    if (segment_end_secs_.empty()) {
        position_ = route_.back().ecef;
        return;
    }

    double elapsed = static_cast<double>(time_sec - start_sec_);
    if (elapsed >= total_duration_sec_) {
        position_ = route_.back().ecef;
        return;
    }

    auto it = std::upper_bound(segment_end_secs_.begin(), segment_end_secs_.end(), elapsed);
    size_t segment_index = static_cast<size_t>(std::distance(segment_end_secs_.begin(), it));
    if (segment_index >= segment_end_secs_.size()) {
        position_ = route_.back().ecef;
        return;
    }

    double segment_end = segment_end_secs_[segment_index];
    double segment_start = (segment_index == 0) ? 0.0 : segment_end_secs_[segment_index - 1];
    double segment_duration = segment_end - segment_start;
    if (segment_duration <= 0.0) {
        position_ = route_[segment_index + 1].ecef;
        return;
    }
    if (!std::isfinite(segment_duration)) {
        position_ = route_[segment_index].ecef;
        return;
    }

    double t = (elapsed - segment_start) / segment_duration;
    const Ecef &a = route_[segment_index].ecef;
    const Ecef &b = route_[segment_index + 1].ecef;
    position_ = Ecef{
        a.x + (b.x - a.x) * t,
        a.y + (b.y - a.y) * t,
        a.z + (b.z - a.z) * t,
    };
}
