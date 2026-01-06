#include "catch_amalgamated.hpp"

#include "commander_object.hpp"

TEST_CASE("CommanderObjectは固定オブジェクトとして位置が変わらないこと", "[commander_object]") {
    // 司令官は移動しない設計なので、位置が常に最初の経路点になることを確認します。
    RoutePoint point;
    point.ecef = Ecef{5.0, 6.0, 7.0};

    std::vector<RoutePoint> route{point};
    CommanderObject obj("cmd-1", "team-a", simoop::Role::COMMANDER, 0, route, {});

    obj.updatePosition(100);

    REQUIRE(obj.position().x == Catch::Approx(5.0));
    REQUIRE(obj.position().y == Catch::Approx(6.0));
    REQUIRE(obj.position().z == Catch::Approx(7.0));
}
