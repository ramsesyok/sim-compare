//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     DetectionEvent data = nlohmann::json::parse(jsonString);

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
     * 探知アクション
     *
     * 探知行動種別
     */
    enum class DetectionAction : int
    {
        FOUND,
        LOST
    };

    /**
     * 探知イベント
     */
    class DetectionEvent
    {
    public:
        DetectionEvent() = default;
        virtual ~DetectionEvent() = default;

    private:
        double altM;
        std::string detectId;
        DetectionAction detectionAction;
        int64_t distanceM;
        std::string eventType;
        double latDeg;
        double lonDeg;
        std::string scountId;
        int64_t timeSec;

    public:
        /**
         * 探知位置の高度(m)
         */
        const double &getAltM() const { return altM; }
        double &getMutableAltM() { return altM; }
        void setAltM(const double &value) { this->altM = value; }

        /**
         * 探知対象ID
         */
        const std::string &getDetectId() const { return detectId; }
        std::string &getMutableDetectId() { return detectId; }
        void setDetectId(const std::string &value) { this->detectId = value; }

        /**
         * 探知アクション
         */
        const DetectionAction &getDetectionAction() const { return detectionAction; }
        DetectionAction &getMutableDetectionAction() { return detectionAction; }
        void setDetectionAction(const DetectionAction &value) { this->detectionAction = value; }

        /**
         * 探知対象までの距離(m)
         */
        const int64_t &getDistanceM() const { return distanceM; }
        int64_t &getMutableDistanceM() { return distanceM; }
        void setDistanceM(const int64_t &value) { this->distanceM = value; }

        /**
         * イベント種別
         */
        const std::string &getEventType() const { return eventType; }
        std::string &getMutableEventType() { return eventType; }
        void setEventType(const std::string &value) { this->eventType = value; }

        /**
         * 探知位置の緯度(度)
         */
        const double &getLatDeg() const { return latDeg; }
        double &getMutableLatDeg() { return latDeg; }
        void setLatDeg(const double &value) { this->latDeg = value; }

        /**
         * 探知位置の経度(度)
         */
        const double &getLonDeg() const { return lonDeg; }
        double &getMutableLonDeg() { return lonDeg; }
        void setLonDeg(const double &value) { this->lonDeg = value; }

        /**
         * 斥候役ID
         */
        const std::string &getScountId() const { return scountId; }
        std::string &getMutableScountId() { return scountId; }
        void setScountId(const std::string &value) { this->scountId = value; }

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
    void from_json(const json &j, DetectionEvent &x);
    void to_json(json &j, const DetectionEvent &x);

    void from_json(const json &j, DetectionAction &x);
    void to_json(json &j, const DetectionAction &x);

    /**
     * @brief DetectionEventをJSONから生成します。
     */
    inline void from_json(const json &j, DetectionEvent &x)
    {
        x.setAltM(j.at("alt_m").get<double>());
        x.setDetectId(j.at("detect_id").get<std::string>());
        x.setDetectionAction(j.at("detection_action").get<DetectionAction>());
        x.setDistanceM(j.at("distance_m").get<int64_t>());
        x.setEventType(j.at("event_type").get<std::string>());
        x.setLatDeg(j.at("lat_deg").get<double>());
        x.setLonDeg(j.at("lon_deg").get<double>());
        x.setScountId(j.at("scount_id").get<std::string>());
        x.setTimeSec(j.at("time_sec").get<int64_t>());
    }

    /**
     * @brief DetectionEventをJSONへ変換します。
     */
    inline void to_json(json &j, const DetectionEvent &x)
    {
        j = json::object();
        j["alt_m"] = x.getAltM();
        j["detect_id"] = x.getDetectId();
        j["detection_action"] = x.getDetectionAction();
        j["distance_m"] = x.getDistanceM();
        j["event_type"] = x.getEventType();
        j["lat_deg"] = x.getLatDeg();
        j["lon_deg"] = x.getLonDeg();
        j["scount_id"] = x.getScountId();
        j["time_sec"] = x.getTimeSec();
    }

    /**
     * @brief DetectionAction列挙体をJSONから生成します。
     */
    inline void from_json(const json &j, DetectionAction &x)
    {
        if (j == "found")
            x = DetectionAction::FOUND;
        else if (j == "lost")
            x = DetectionAction::LOST;
        else
        {
            throw std::runtime_error("Input JSON does not conform to schema!");
        }
    }

    /**
     * @brief DetectionAction列挙体をJSONへ変換します。
     */
    inline void to_json(json &j, const DetectionAction &x)
    {
        switch (x)
        {
        case DetectionAction::FOUND:
            j = "found";
            break;
        case DetectionAction::LOST:
            j = "lost";
            break;
        default:
            throw std::runtime_error("Unexpected value in enumeration \"DetectionAction\": " + std::to_string(static_cast<int>(x)));
        }
    }
}
