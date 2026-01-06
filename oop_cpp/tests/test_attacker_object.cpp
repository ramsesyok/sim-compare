#include "catch_amalgamated.hpp"

#include <sstream>

#include "attacker_object.hpp"

TEST_CASE("AttackerObjectは到達後に一度だけ爆破ログを出力すること", "[attacker_object]") {
    // 爆破イベントが一度だけ出力されることを確認し、二重出力を防ぐ設計を検証します。
    RoutePoint a;
    a.ecef = Ecef{0.0, 0.0, 0.0};
    RoutePoint b;
    b.ecef = Ecef{10.0, 0.0, 0.0};

    std::vector<RoutePoint> route{a, b};
    std::vector<double> segment_end_secs{1.0};

    AttackerObject obj("atk-1", "team-a", 0, route, {}, segment_end_secs, 1.0, 500);

    obj.updatePosition(1);

    std::ostringstream out;
    obj.emitDetonation(1, out);
    std::string first = out.str();

    obj.emitDetonation(2, out);
    std::string second = out.str();

    REQUIRE_FALSE(first.empty());
    REQUIRE(first == second);
}
