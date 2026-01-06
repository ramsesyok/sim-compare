#include "catch_amalgamated.hpp"

#include "messenger_object.hpp"

TEST_CASE("MessengerObjectは移動ロジックを継承して動くこと", "[messenger_object]") {
    // 伝令役はMovableObjectを継承するため、移動挙動が共有されていることを確認します。
    RoutePoint a;
    a.ecef = Ecef{0.0, 0.0, 0.0};
    RoutePoint b;
    b.ecef = Ecef{0.0, 10.0, 0.0};

    std::vector<RoutePoint> route{a, b};
    std::vector<double> segment_end_secs{2.0};

    MessengerObject obj("msg-1", "team-a", 0, route, {}, segment_end_secs, 2.0, 1000);

    obj.updatePosition(1);
    REQUIRE(obj.position().y == Catch::Approx(5.0));
}
