#include "catch_amalgamated.hpp"

#include "geo.hpp"
#include "route.hpp"
#include "simulation.hpp"

TEST_CASE("ECEF変換が基本的なケースで安定すること", "[geo]") {
    // 初心者が追いやすいように、赤道・経度0度の位置を基準にします。
    double lat_deg = 0.0;
    double lon_deg = 0.0;
    double alt_m = 0.0;

    Ecef ecef = geodeticToEcef(lat_deg, lon_deg, alt_m);

    // 期待値は地球半径付近であることを確認し、逆変換で元の座標に戻るかを見ます。
    REQUIRE(ecef.x == Catch::Approx(6378137.0).margin(1e-3));
    REQUIRE(ecef.y == Catch::Approx(0.0).margin(1e-6));
    REQUIRE(ecef.z == Catch::Approx(0.0).margin(1e-6));

    double out_lat = 0.0;
    double out_lon = 0.0;
    double out_alt = 0.0;
    ecefToGeodetic(ecef, out_lat, out_lon, out_alt);

    REQUIRE(out_lat == Catch::Approx(lat_deg).margin(1e-6));
    REQUIRE(out_lon == Catch::Approx(lon_deg).margin(1e-6));
    REQUIRE(out_alt == Catch::Approx(alt_m).margin(1e-3));
}

TEST_CASE("移動区間時間の計算が速度に一致すること", "[route]") {
    // 区間距離10mを速度10m/sで移動すると1秒になることを確認します。
    RoutePoint a;
    a.ecef = Ecef{0.0, 0.0, 0.0};
    a.speeds_kph = 36.0;

    RoutePoint b;
    b.ecef = Ecef{10.0, 0.0, 0.0};
    b.speeds_kph = 36.0;

    std::vector<RoutePoint> route{a, b};
    auto result = buildSegmentTimes(route);

    REQUIRE(result.first.size() == 1);
    REQUIRE(result.first[0] == Catch::Approx(1.0).margin(1e-6));
    REQUIRE(result.second == Catch::Approx(1.0).margin(1e-6));
}

TEST_CASE("役割の文字列化が期待通りであること", "[simulation]") {
    // ログ出力で使う役割文字列が変わらないことを確認します。
    Simulation simulation;

    REQUIRE(simulation.roleToString(simoop::Role::COMMANDER) == "commander");
    REQUIRE(simulation.roleToString(simoop::Role::SCOUT) == "scout");
    REQUIRE(simulation.roleToString(simoop::Role::MESSENGER) == "messenger");
    REQUIRE(simulation.roleToString(simoop::Role::ATTACKER) == "attacker");
}
