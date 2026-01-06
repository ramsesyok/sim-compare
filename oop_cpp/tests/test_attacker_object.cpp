#include "catch_amalgamated.hpp"

#include <filesystem>
#include <fstream>

#include "attacker_object.hpp"
#include "logging.hpp"

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

    auto path = std::filesystem::temp_directory_path() / "sim_compare_attacker_event.log";
    EventLogger::instance().open(path.string());

    obj.emitDetonation(1);
    obj.emitDetonation(2);

    std::ifstream in(path);
    std::string line1;
    std::string line2;
    std::getline(in, line1);
    std::getline(in, line2);

    EventLogger::instance().close();

    REQUIRE_FALSE(line1.empty());
    REQUIRE(line2.empty());
}
