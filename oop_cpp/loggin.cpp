#include "loggin.hpp"

#include "geo.hpp"
#include "sim_object.hpp"
#include "simulation.hpp"
#include "timeline.hpp"

void writeTimelineLog(int time_sec,
                      const std::vector<SimObject *> &objects,
                      const Simulation &simulation,
                      std::ostream &out) {
    // タイムラインログは1秒ごとの全オブジェクト位置をまとめて出力します。
    simoop::Timeline timeline;
    timeline.setTimeSec(time_sec);
    std::vector<simoop::TimelinePosition> positions;
    positions.reserve(objects.size());
    for (size_t i = 0; i < objects.size(); ++i) {
        const SimObject *obj = objects[i];
        double lat = 0.0;
        double lon = 0.0;
        double alt = 0.0;
        ecefToGeodetic(obj->position(), lat, lon, alt);
        simoop::TimelinePosition position;
        position.setObjectId(obj->id());
        position.setTeamId(obj->teamId());
        position.setRole(simulation.roleToString(obj->role()));
        position.setLatDeg(lat);
        position.setLonDeg(lon);
        position.setAltM(alt);
        positions.push_back(position);
    }
    timeline.setPositions(positions);
    nlohmann::json json_timeline;
    simoop::to_json(json_timeline, timeline);
    out << json_timeline.dump() << '\n';
}
