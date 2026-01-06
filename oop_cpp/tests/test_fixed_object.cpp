#include "catch_amalgamated.hpp"

#include "fixed_object.hpp"
#include "geo.hpp"
#include "route.hpp"

TEST_CASE("FixedObjectは常に最初の経路点に固定されること", "[fixed_object]") {
    // 固定オブジェクトの動作確認として、更新後も位置が変わらないことを検証します。
    RoutePoint point;
    point.ecef = Ecef{100.0, 200.0, 300.0};

    std::vector<RoutePoint> route{point};
    FixedObject obj("fixed-1", "team-a", simoop::Role::COMMANDER, 0, route, {});

    obj.updatePosition(0);

    REQUIRE(obj.position().x == Catch::Approx(100.0));
    REQUIRE(obj.position().y == Catch::Approx(200.0));
    REQUIRE(obj.position().z == Catch::Approx(300.0));
}
