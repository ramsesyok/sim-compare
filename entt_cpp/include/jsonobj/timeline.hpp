/**
 * @file timeline.hpp
 * @brief タイムライン出力JSONの型定義をまとめたヘッダです。
 *
 * @details 1秒ごとの位置ログをC++クラスに対応させ、読み書きを容易にします。
 */
//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     Timeline data = nlohmann::json::parse(jsonString);

#pragma once

#include "nlohmann/json.hpp"

#include <optional>
#include <stdexcept>
#include <regex>

namespace jsonobj
{
    using nlohmann::json;

#ifndef NLOHMANN_UNTYPED_jsonobj_HELPER
#define NLOHMANN_UNTYPED_jsonobj_HELPER
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
     * @brief タイムライン出力に含まれる1オブジェクト分の位置情報です。
     */
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
        /**
         * @brief 高度(m)を取得します。
         */
        const double &getAltM() const { return altM; }
        /**
         * @brief 高度(m)を変更可能な参照として取得します。
         */
        double &getMutableAltM() { return altM; }
        /**
         * @brief 高度(m)を設定します。
         */
        void setAltM(const double &value) { this->altM = value; }

        /**
         * @brief 緯度(度)を取得します。
         */
        const double &getLatDeg() const { return latDeg; }
        /**
         * @brief 緯度(度)を変更可能な参照として取得します。
         */
        double &getMutableLatDeg() { return latDeg; }
        /**
         * @brief 緯度(度)を設定します。
         */
        void setLatDeg(const double &value) { this->latDeg = value; }

        /**
         * @brief 経度(度)を取得します。
         */
        const double &getLonDeg() const { return lonDeg; }
        /**
         * @brief 経度(度)を変更可能な参照として取得します。
         */
        double &getMutableLonDeg() { return lonDeg; }
        /**
         * @brief 経度(度)を設定します。
         */
        void setLonDeg(const double &value) { this->lonDeg = value; }

        /**
         * @brief オブジェクトIDを取得します。
         */
        const std::string &getObjectId() const { return objectId; }
        /**
         * @brief オブジェクトIDを変更可能な参照として取得します。
         */
        std::string &getMutableObjectId() { return objectId; }
        /**
         * @brief オブジェクトIDを設定します。
         */
        void setObjectId(const std::string &value) { this->objectId = value; }

        /**
         * @brief 役割文字列を取得します。
         */
        const std::string &getRole() const { return role; }
        /**
         * @brief 役割文字列を変更可能な参照として取得します。
         */
        std::string &getMutableRole() { return role; }
        /**
         * @brief 役割文字列を設定します。
         */
        void setRole(const std::string &value) { this->role = value; }

        /**
         * @brief チームIDを取得します。
         */
        const std::string &getTeamId() const { return teamId; }
        /**
         * @brief チームIDを変更可能な参照として取得します。
         */
        std::string &getMutableTeamId() { return teamId; }
        /**
         * @brief チームIDを設定します。
         */
        void setTeamId(const std::string &value) { this->teamId = value; }
    };

    /**
     * @brief 1秒分のタイムラインログをまとめたクラスです。
     */
    class Timeline
    {
    public:
        Timeline() = default;
        virtual ~Timeline() = default;

    private:
        std::vector<TimelinePosition> positions;
        int64_t timeSec;

    public:
        /**
         * @brief 位置情報一覧を取得します。
         */
        const std::vector<TimelinePosition> &getPositions() const { return positions; }
        /**
         * @brief 位置情報一覧を変更可能な参照として取得します。
         */
        std::vector<TimelinePosition> &getMutablePositions() { return positions; }
        /**
         * @brief 位置情報一覧を設定します。
         */
        void setPositions(const std::vector<TimelinePosition> &value) { this->positions = value; }

        /**
         * @brief 時刻(秒)を取得します。
         */
        const int64_t &getTimeSec() const { return timeSec; }
        /**
         * @brief 時刻(秒)を変更可能な参照として取得します。
         */
        int64_t &getMutableTimeSec() { return timeSec; }
        /**
         * @brief 時刻(秒)を設定します。
         */
        void setTimeSec(const int64_t &value) { this->timeSec = value; }
    };
}

namespace jsonobj
{
    void from_json(const json &j, TimelinePosition &x);
    void to_json(json &j, const TimelinePosition &x);

    void from_json(const json &j, Timeline &x);
    void to_json(json &j, const Timeline &x);

    /**
     * @brief TimelinePositionをJSONから生成します。
     */
    inline void from_json(const json &j, TimelinePosition &x)
    {
        x.setAltM(j.at("alt_m").get<double>());
        x.setLatDeg(j.at("lat_deg").get<double>());
        x.setLonDeg(j.at("lon_deg").get<double>());
        x.setObjectId(j.at("object_id").get<std::string>());
        x.setRole(j.at("role").get<std::string>());
        x.setTeamId(j.at("team_id").get<std::string>());
    }

    /**
     * @brief TimelinePositionをJSONへ変換します。
     */
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

    /**
     * @brief TimelineをJSONから生成します。
     */
    inline void from_json(const json &j, Timeline &x)
    {
        x.setPositions(j.at("positions").get<std::vector<TimelinePosition>>());
        x.setTimeSec(j.at("time_sec").get<int64_t>());
    }

    /**
     * @brief TimelineをJSONへ変換します。
     */
    inline void to_json(json &j, const Timeline &x)
    {
        j = json::object();
        j["positions"] = x.getPositions();
        j["time_sec"] = x.getTimeSec();
    }
}
