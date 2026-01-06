//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     Timeline data = nlohmann::json::parse(jsonString);

#pragma once

#include "json.hpp"

#include <optional>
#include <stdexcept>
#include <regex>

namespace simoop
{
    using nlohmann::json;

#ifndef NLOHMANN_UNTYPED_simoop_HELPER
#define NLOHMANN_UNTYPED_simoop_HELPER
    inline json get_untyped(const json &j, const char *property)
    {
        if (j.find(property) != j.end())
        {
            return j.at(property).get<json>();
        }
        return json();
    }

    inline json get_untyped(const json &j, std::string property)
    {
        return get_untyped(j, property.data());
    }
#endif

    class TimelinePosition
    {
    public:
        TimelinePosition() = default;
        virtual ~TimelinePosition() = default;

    private:
        double altM;
        double latDeg;
        double lonDeg;
        std::string objectId;
        std::string role;
        std::string teamId;

    public:
        const double &getAltM() const { return altM; }
        double &getMutableAltM() { return altM; }
        void setAltM(const double &value) { this->altM = value; }

        const double &getLatDeg() const { return latDeg; }
        double &getMutableLatDeg() { return latDeg; }
        void setLatDeg(const double &value) { this->latDeg = value; }

        const double &getLonDeg() const { return lonDeg; }
        double &getMutableLonDeg() { return lonDeg; }
        void setLonDeg(const double &value) { this->lonDeg = value; }

        const std::string &getObjectId() const { return objectId; }
        std::string &getMutableObjectId() { return objectId; }
        void setObjectId(const std::string &value) { this->objectId = value; }

        const std::string &getRole() const { return role; }
        std::string &getMutableRole() { return role; }
        void setRole(const std::string &value) { this->role = value; }

        const std::string &getTeamId() const { return teamId; }
        std::string &getMutableTeamId() { return teamId; }
        void setTeamId(const std::string &value) { this->teamId = value; }
    };

    class Timeline
    {
    public:
        Timeline() = default;
        virtual ~Timeline() = default;

    private:
        std::vector<TimelinePosition> positions;
        int64_t timeSec;

    public:
        const std::vector<TimelinePosition> &getPositions() const { return positions; }
        std::vector<TimelinePosition> &getMutablePositions() { return positions; }
        void setPositions(const std::vector<TimelinePosition> &value) { this->positions = value; }

        const int64_t &getTimeSec() const { return timeSec; }
        int64_t &getMutableTimeSec() { return timeSec; }
        void setTimeSec(const int64_t &value) { this->timeSec = value; }
    };
}

namespace simoop
{
    void from_json(const json &j, TimelinePosition &x);
    void to_json(json &j, const TimelinePosition &x);

    void from_json(const json &j, Timeline &x);
    void to_json(json &j, const Timeline &x);

    inline void from_json(const json &j, TimelinePosition &x)
    {
        x.setAltM(j.at("alt_m").get<double>());
        x.setLatDeg(j.at("lat_deg").get<double>());
        x.setLonDeg(j.at("lon_deg").get<double>());
        x.setObjectId(j.at("object_id").get<std::string>());
        x.setRole(j.at("role").get<std::string>());
        x.setTeamId(j.at("team_id").get<std::string>());
    }

    inline void to_json(json &j, const TimelinePosition &x)
    {
        j = json::object();
        j["alt_m"] = x.getAltM();
        j["lat_deg"] = x.getLatDeg();
        j["lon_deg"] = x.getLonDeg();
        j["object_id"] = x.getObjectId();
        j["role"] = x.getRole();
        j["team_id"] = x.getTeamId();
    }

    inline void from_json(const json &j, Timeline &x)
    {
        x.setPositions(j.at("positions").get<std::vector<TimelinePosition>>());
        x.setTimeSec(j.at("time_sec").get<int64_t>());
    }

    inline void to_json(json &j, const Timeline &x)
    {
        j = json::object();
        j["positions"] = x.getPositions();
        j["time_sec"] = x.getTimeSec();
    }
}
