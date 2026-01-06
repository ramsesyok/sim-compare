#include "catch_amalgamated.hpp"

#include <sstream>

#include "commander_object.hpp"
#include "scout_object.hpp"
#include "spatial_hash.hpp"

TEST_CASE("ScoutObjectは敵が範囲内に入ると探知イベントを出力すること", "[scout_object]") {
    // 斥候の探知はクラス固有の責務なので、イベント出力が行われることを確認します。
    RoutePoint scout_point;
    scout_point.ecef = Ecef{0.0, 0.0, 0.0};
    std::vector<RoutePoint> scout_route{scout_point};

    ScoutObject scout("scout-1", "team-a", 0, scout_route, {}, {}, 0.0, 100, 0);

    RoutePoint enemy_point;
    enemy_point.ecef = Ecef{10.0, 0.0, 0.0};
    std::vector<RoutePoint> enemy_route{enemy_point};
    CommanderObject enemy("enemy-1", "team-b", simoop::Role::COMMANDER, 0, enemy_route, {});

    std::vector<SimObject *> objects{&scout, &enemy};
    auto spatial_hash = buildSpatialHash(objects, 100.0);

    std::ostringstream out;
    scout.updateDetection(0, spatial_hash, objects, 0, out);

    std::string log = out.str();
    REQUIRE(log.find("\"detection_action\":\"found\"") != std::string::npos);
}
