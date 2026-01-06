#include "catch_amalgamated.hpp"

#include "movable_object.hpp"

TEST_CASE("MovableObjectは開始前と完了後で位置が固定されること", "[movable_object]") {
    // 移動開始前は最初の経路点、移動完了後は最終点にいることを確認します。
    RoutePoint a;
    a.ecef = Ecef{0.0, 0.0, 0.0};
    RoutePoint b;
    b.ecef = Ecef{10.0, 0.0, 0.0};

    std::vector<RoutePoint> route{a, b};
    std::vector<double> segment_end_secs{1.0};

    MovableObject obj("move-1", "team-a", simoop::Role::MESSENGER, 5, route, {}, segment_end_secs, 1.0);

    obj.updatePosition(4);
    REQUIRE(obj.position().x == Catch::Approx(0.0));

    obj.updatePosition(6);
    REQUIRE(obj.position().x == Catch::Approx(10.0));
}
