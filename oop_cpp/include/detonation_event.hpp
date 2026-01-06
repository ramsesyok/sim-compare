//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     DetonationEvent data = nlohmann::json::parse(jsonString);

#pragma once

#include "nlohmann/json.hpp"

#include <optional>
#include <stdexcept>
#include <regex>

namespace simoop
{
    using nlohmann::json;

#ifndef NLOHMANN_UNTYPED_simoop_HELPER
#define NLOHMANN_UNTYPED_simoop_HELPER
    /**
     * @brief 任意型の値を取得するための補助関数です。
     */
    inline json get_untyped(const json &j, const char *property)
    {
        if (j.find(property) != j.end())
        {
            return j.at(property).get<json>();
        }
        return json();
    }

    /**
     * @brief 文字列キー版の任意型取得ヘルパーです。
     */
    inline json get_untyped(const json &j, std::string property)
    {
        return get_untyped(j, property.data());
    }
#endif

    /**
     * 爆破イベント
     */
    class DetonationEvent
    {
    public:
        DetonationEvent() = default;
        virtual ~DetonationEvent() = default;

    private:
        double altM;
        std::string attackerId;
        int64_t bomRangeM;
        std::string eventType;
        double latDeg;
        double lonDeg;
        int64_t timeSec;

    public:
        /**
         * 爆破位置の高度(m)
         */
        const double &getAltM() const { return altM; }
        double &getMutableAltM() { return altM; }
        void setAltM(const double &value) { this->altM = value; }

        /**
         * 攻撃役ID
         */
        const std::string &getAttackerId() const { return attackerId; }
        std::string &getMutableAttackerId() { return attackerId; }
        void setAttackerId(const std::string &value) { this->attackerId = value; }

        /**
         * 爆破範囲(m)
         */
        const int64_t &getBomRangeM() const { return bomRangeM; }
        int64_t &getMutableBomRangeM() { return bomRangeM; }
        void setBomRangeM(const int64_t &value) { this->bomRangeM = value; }

        /**
         * イベント種別
         */
        const std::string &getEventType() const { return eventType; }
        std::string &getMutableEventType() { return eventType; }
        void setEventType(const std::string &value) { this->eventType = value; }

        /**
         * 爆破位置の緯度(度)
         */
        const double &getLatDeg() const { return latDeg; }
        double &getMutableLatDeg() { return latDeg; }
        void setLatDeg(const double &value) { this->latDeg = value; }

        /**
         * 爆破位置の経度(度)
         */
        const double &getLonDeg() const { return lonDeg; }
        double &getMutableLonDeg() { return lonDeg; }
        void setLonDeg(const double &value) { this->lonDeg = value; }

        /**
         * イベント発生時刻(秒)
         */
        const int64_t &getTimeSec() const { return timeSec; }
        int64_t &getMutableTimeSec() { return timeSec; }
        void setTimeSec(const int64_t &value) { this->timeSec = value; }
    };
}

namespace simoop
{
    void from_json(const json &j, DetonationEvent &x);
    void to_json(json &j, const DetonationEvent &x);

    /**
     * @brief DetonationEventをJSONから生成します。
     */
    inline void from_json(const json &j, DetonationEvent &x)
    {
        x.setAltM(j.at("alt_m").get<double>());
        x.setAttackerId(j.at("attacker_id").get<std::string>());
        x.setBomRangeM(j.at("bom_range_m").get<int64_t>());
        x.setEventType(j.at("event_type").get<std::string>());
        x.setLatDeg(j.at("lat_deg").get<double>());
        x.setLonDeg(j.at("lon_deg").get<double>());
        x.setTimeSec(j.at("time_sec").get<int64_t>());
    }

    /**
     * @brief DetonationEventをJSONへ変換します。
     */
    inline void to_json(json &j, const DetonationEvent &x)
    {
        j = json::object();
        j["alt_m"] = x.getAltM();
        j["attacker_id"] = x.getAttackerId();
        j["bom_range_m"] = x.getBomRangeM();
        j["event_type"] = x.getEventType();
        j["lat_deg"] = x.getLatDeg();
        j["lon_deg"] = x.getLonDeg();
        j["time_sec"] = x.getTimeSec();
    }
}
