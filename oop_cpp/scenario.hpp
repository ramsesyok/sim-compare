//  To parse this JSON data, first install
//
//      json.hpp  https://github.com/nlohmann/json
//
//  Then include this file, and then do
//
//     Scenario data = nlohmann::json::parse(jsonString);

#pragma once

#include <optional>
#include "json.hpp"

#include <optional>
#include <stdexcept>
#include <regex>

#ifndef NLOHMANN_OPT_HELPER
#define NLOHMANN_OPT_HELPER
namespace nlohmann
{
    template <typename T>
    /**
     * @brief shared_ptr用のJSON変換を定義するシリアライザです。
     */
    struct adl_serializer<std::shared_ptr<T>>
    {
        /**
         * @brief shared_ptrをJSONへ変換します。
         */
        static void to_json(json &j, const std::shared_ptr<T> &opt)
        {
            if (!opt)
                j = nullptr;
            else
                j = *opt;
        }

        /**
         * @brief JSONからshared_ptrを復元します。
         */
        static std::shared_ptr<T> from_json(const json &j)
        {
            if (j.is_null())
                return std::make_shared<T>();
            else
                return std::make_shared<T>(j.get<T>());
        }
    };
    template <typename T>
    /**
     * @brief optional用のJSON変換を定義するシリアライザです。
     */
    struct adl_serializer<std::optional<T>>
    {
        /**
         * @brief optionalをJSONへ変換します。
         */
        static void to_json(json &j, const std::optional<T> &opt)
        {
            if (!opt)
                j = nullptr;
            else
                j = *opt;
        }

        /**
         * @brief JSONからoptionalを復元します。
         */
        static std::optional<T> from_json(const json &j)
        {
            if (j.is_null())
                return std::make_optional<T>();
            else
                return std::make_optional<T>(j.get<T>());
        }
    };
}
#endif

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

#ifndef NLOHMANN_OPTIONAL_simoop_HELPER
#define NLOHMANN_OPTIONAL_simoop_HELPER
    template <typename T>
    /**
     * @brief optional値をshared_ptrとして取得するヘルパーです。
     */
    inline std::shared_ptr<T> get_heap_optional(const json &j, const char *property)
    {
        auto it = j.find(property);
        if (it != j.end() && !it->is_null())
        {
            return j.at(property).get<std::shared_ptr<T>>();
        }
        return std::shared_ptr<T>();
    }

    template <typename T>
    /**
     * @brief 文字列キー版のshared_ptr取得ヘルパーです。
     */
    inline std::shared_ptr<T> get_heap_optional(const json &j, std::string property)
    {
        return get_heap_optional<T>(j, property.data());
    }
    template <typename T>
    /**
     * @brief optional値をスタック上のoptionalとして取得するヘルパーです。
     */
    inline std::optional<T> get_stack_optional(const json &j, const char *property)
    {
        auto it = j.find(property);
        if (it != j.end() && !it->is_null())
        {
            return j.at(property).get<std::optional<T>>();
        }
        return std::optional<T>();
    }

    template <typename T>
    /**
     * @brief 文字列キー版のoptional取得ヘルパーです。
     */
    inline std::optional<T> get_stack_optional(const json &j, std::string property)
    {
        return get_stack_optional<T>(j, property.data());
    }
#endif

    /**
     * 攻撃役の性能諸元
     *
     * 攻撃役の攻撃性能など
     */
    class Attacker
    {
    public:
        Attacker() = default;
        virtual ~Attacker() = default;

    private:
        int64_t bomRangeM;

    public:
        /**
         * 攻撃影響範囲(m)
         */
        const int64_t &getBomRangeM() const { return bomRangeM; }
        int64_t &getMutableBomRangeM() { return bomRangeM; }
        void setBomRangeM(const int64_t &value) { this->bomRangeM = value; }
    };

    /**
     * 伝令役の性能諸元
     *
     * 伝令役の通信性能など
     */
    class Messenger
    {
    public:
        Messenger() = default;
        virtual ~Messenger() = default;

    private:
        int64_t commRangeM;

    public:
        /**
         * 通信可能範囲(m)
         */
        const int64_t &getCommRangeM() const { return commRangeM; }
        int64_t &getMutableCommRangeM() { return commRangeM; }
        void setCommRangeM(const int64_t &value) { this->commRangeM = value; }
    };

    /**
     * 斥候役の性能諸元
     *
     * 斥候役の探知性能・通知性能など
     */
    class Scout
    {
    public:
        Scout() = default;
        virtual ~Scout() = default;

    private:
        int64_t commRangeM;
        int64_t detectRangeM;

    public:
        /**
         * 通信可能範囲(m)
         */
        const int64_t &getCommRangeM() const { return commRangeM; }
        int64_t &getMutableCommRangeM() { return commRangeM; }
        void setCommRangeM(const int64_t &value) { this->commRangeM = value; }

        /**
         * 探知可能範囲(m)
         */
        const int64_t &getDetectRangeM() const { return detectRangeM; }
        int64_t &getMutableDetectRangeM() { return detectRangeM; }
        void setDetectRangeM(const int64_t &value) { this->detectRangeM = value; }
    };

    /**
     * 性能諸元
     */
    class Performance
    {
    public:
        Performance() = default;
        virtual ~Performance() = default;

    private:
        Attacker attacker;
        Messenger messenger;
        Scout scout;

    public:
        /**
         * 攻撃役の性能諸元
         */
        const Attacker &getAttacker() const { return attacker; }
        Attacker &getMutableAttacker() { return attacker; }
        void setAttacker(const Attacker &value) { this->attacker = value; }

        /**
         * 伝令役の性能諸元
         */
        const Messenger &getMessenger() const { return messenger; }
        Messenger &getMutableMessenger() { return messenger; }
        void setMessenger(const Messenger &value) { this->messenger = value; }

        /**
         * 斥候役の性能諸元
         */
        const Scout &getScout() const { return scout; }
        Scout &getMutableScout() { return scout; }
        void setScout(const Scout &value) { this->scout = value; }
    };

    /**
     * オブジェクトの役割
     */
    enum class Role : int
    {
        ATTACKER,
        COMMANDER,
        MESSENGER,
        SCOUT
    };

    /**
     * 移動経路の経路点情報
     */
    class Waypoint
    {
    public:
        Waypoint() = default;
        virtual ~Waypoint() = default;

    private:
        double altM;
        double latDeg;
        double lonDeg;
        double speedsKph;

    public:
        /**
         * 高度(m)
         */
        const double &getAltM() const { return altM; }
        double &getMutableAltM() { return altM; }
        void setAltM(const double &value) { this->altM = value; }

        /**
         * 緯度(度)
         */
        const double &getLatDeg() const { return latDeg; }
        double &getMutableLatDeg() { return latDeg; }
        void setLatDeg(const double &value) { this->latDeg = value; }

        /**
         * 経度(度)
         */
        const double &getLonDeg() const { return lonDeg; }
        double &getMutableLonDeg() { return lonDeg; }
        void setLonDeg(const double &value) { this->lonDeg = value; }

        /**
         * 移動速度(km/h)
         */
        const double &getSpeedsKph() const { return speedsKph; }
        double &getMutableSpeedsKph() { return speedsKph; }
        void setSpeedsKph(const double &value) { this->speedsKph = value; }
    };

    /**
     * オブジェクトシナリオ情報
     */
    class Object
    {
    public:
        Object() = default;
        virtual ~Object() = default;

    private:
        std::string id;
        std::optional<std::vector<std::string>> network;
        Role role;
        std::vector<Waypoint> route;
        int64_t startSec;

    public:
        /**
         * オブジェクト識別子
         */
        const std::string &getId() const { return id; }
        std::string &getMutableId() { return id; }
        void setId(const std::string &value) { this->id = value; }

        /**
         * オブジェクトが利用可能な伝令役のオブジェクトID一覧
         */
        std::optional<std::vector<std::string>> getNetwork() const { return network; }
        void setNetwork(std::optional<std::vector<std::string>> value) { this->network = value; }

        /**
         * オブジェクトの役割
         */
        const Role &getRole() const { return role; }
        Role &getMutableRole() { return role; }
        void setRole(const Role &value) { this->role = value; }

        /**
         * 移動経路情報
         */
        const std::vector<Waypoint> &getRoute() const { return route; }
        std::vector<Waypoint> &getMutableRoute() { return route; }
        void setRoute(const std::vector<Waypoint> &value) { this->route = value; }

        /**
         * シナリオ開始からの移動開始時間(秒)
         */
        const int64_t &getStartSec() const { return startSec; }
        int64_t &getMutableStartSec() { return startSec; }
        void setStartSec(const int64_t &value) { this->startSec = value; }
    };

    /**
     * チーム情報
     */
    class Team
    {
    public:
        Team() = default;
        virtual ~Team() = default;

    private:
        std::string id;
        std::string name;
        std::vector<Object> objects;

    public:
        /**
         * チーム識別子
         */
        const std::string &getId() const { return id; }
        std::string &getMutableId() { return id; }
        void setId(const std::string &value) { this->id = value; }

        /**
         * チーム名
         */
        const std::string &getName() const { return name; }
        std::string &getMutableName() { return name; }
        void setName(const std::string &value) { this->name = value; }

        const std::vector<Object> &getObjects() const { return objects; }
        std::vector<Object> &getMutableObjects() { return objects; }
        void setObjects(const std::vector<Object> &value) { this->objects = value; }
    };

    /**
     * シナリオ全体情報
     */
    class Scenario
    {
    public:
        Scenario() = default;
        virtual ~Scenario() = default;

    private:
        Performance performance;
        std::vector<Team> teams;

    public:
        /**
         * 性能諸元
         */
        const Performance &getPerformance() const { return performance; }
        Performance &getMutablePerformance() { return performance; }
        void setPerformance(const Performance &value) { this->performance = value; }

        /**
         * チーム情報
         */
        const std::vector<Team> &getTeams() const { return teams; }
        std::vector<Team> &getMutableTeams() { return teams; }
        void setTeams(const std::vector<Team> &value) { this->teams = value; }
    };
}

namespace simoop
{

    void from_json(const json &j, Attacker &x);
    void to_json(json &j, const Attacker &x);

    void from_json(const json &j, Messenger &x);
    void to_json(json &j, const Messenger &x);

    void from_json(const json &j, Scout &x);
    void to_json(json &j, const Scout &x);

    void from_json(const json &j, Performance &x);
    void to_json(json &j, const Performance &x);

    void from_json(const json &j, Waypoint &x);
    void to_json(json &j, const Waypoint &x);

    void from_json(const json &j, Object &x);
    void to_json(json &j, const Object &x);

    void from_json(const json &j, Team &x);
    void to_json(json &j, const Team &x);

    void from_json(const json &j, Scenario &x);
    void to_json(json &j, const Scenario &x);

    void from_json(const json &j, Role &x);
    void to_json(json &j, const Role &x);

    /**
     * @brief AttackerをJSONから生成します。
     */
    inline void from_json(const json &j, Attacker &x)
    {
        x.setBomRangeM(j.at("bom_range_m").get<int64_t>());
    }

    /**
     * @brief AttackerをJSONへ変換します。
     */
    inline void to_json(json &j, const Attacker &x)
    {
        j = json::object();
        j["bom_range_m"] = x.getBomRangeM();
    }

    /**
     * @brief MessengerをJSONから生成します。
     */
    inline void from_json(const json &j, Messenger &x)
    {
        x.setCommRangeM(j.at("comm_range_m").get<int64_t>());
    }

    /**
     * @brief MessengerをJSONへ変換します。
     */
    inline void to_json(json &j, const Messenger &x)
    {
        j = json::object();
        j["comm_range_m"] = x.getCommRangeM();
    }

    /**
     * @brief ScoutをJSONから生成します。
     */
    inline void from_json(const json &j, Scout &x)
    {
        x.setCommRangeM(j.at("comm_range_m").get<int64_t>());
        x.setDetectRangeM(j.at("detect_range_m").get<int64_t>());
    }

    /**
     * @brief ScoutをJSONへ変換します。
     */
    inline void to_json(json &j, const Scout &x)
    {
        j = json::object();
        j["comm_range_m"] = x.getCommRangeM();
        j["detect_range_m"] = x.getDetectRangeM();
    }

    /**
     * @brief PerformanceをJSONから生成します。
     */
    inline void from_json(const json &j, Performance &x)
    {
        x.setAttacker(j.at("attacker").get<Attacker>());
        x.setMessenger(j.at("messenger").get<Messenger>());
        x.setScout(j.at("scout").get<Scout>());
    }

    /**
     * @brief PerformanceをJSONへ変換します。
     */
    inline void to_json(json &j, const Performance &x)
    {
        j = json::object();
        j["attacker"] = x.getAttacker();
        j["messenger"] = x.getMessenger();
        j["scout"] = x.getScout();
    }

    /**
     * @brief WaypointをJSONから生成します。
     */
    inline void from_json(const json &j, Waypoint &x)
    {
        x.setAltM(j.at("alt_m").get<double>());
        x.setLatDeg(j.at("lat_deg").get<double>());
        x.setLonDeg(j.at("lon_deg").get<double>());
        x.setSpeedsKph(j.at("speeds_kph").get<double>());
    }

    /**
     * @brief WaypointをJSONへ変換します。
     */
    inline void to_json(json &j, const Waypoint &x)
    {
        j = json::object();
        j["alt_m"] = x.getAltM();
        j["lat_deg"] = x.getLatDeg();
        j["lon_deg"] = x.getLonDeg();
        j["speeds_kph"] = x.getSpeedsKph();
    }

    /**
     * @brief ObjectをJSONから生成します。
     */
    inline void from_json(const json &j, Object &x)
    {
        x.setId(j.at("id").get<std::string>());
        x.setNetwork(get_stack_optional<std::vector<std::string>>(j, "network"));
        x.setRole(j.at("role").get<Role>());
        x.setRoute(j.at("route").get<std::vector<Waypoint>>());
        x.setStartSec(j.at("start_sec").get<int64_t>());
    }

    /**
     * @brief ObjectをJSONへ変換します。
     */
    inline void to_json(json &j, const Object &x)
    {
        j = json::object();
        j["id"] = x.getId();
        j["network"] = x.getNetwork();
        j["role"] = x.getRole();
        j["route"] = x.getRoute();
        j["start_sec"] = x.getStartSec();
    }

    /**
     * @brief TeamをJSONから生成します。
     */
    inline void from_json(const json &j, Team &x)
    {
        x.setId(j.at("id").get<std::string>());
        x.setName(j.at("name").get<std::string>());
        x.setObjects(j.at("objects").get<std::vector<Object>>());
    }

    /**
     * @brief TeamをJSONへ変換します。
     */
    inline void to_json(json &j, const Team &x)
    {
        j = json::object();
        j["id"] = x.getId();
        j["name"] = x.getName();
        j["objects"] = x.getObjects();
    }

    /**
     * @brief ScenarioをJSONから生成します。
     */
    inline void from_json(const json &j, Scenario &x)
    {
        x.setPerformance(j.at("performance").get<Performance>());
        x.setTeams(j.at("teams").get<std::vector<Team>>());
    }

    /**
     * @brief ScenarioをJSONへ変換します。
     */
    inline void to_json(json &j, const Scenario &x)
    {
        j = json::object();
        j["performance"] = x.getPerformance();
        j["teams"] = x.getTeams();
    }

    /**
     * @brief Role列挙体をJSONから生成します。
     */
    inline void from_json(const json &j, Role &x)
    {
        if (j == "attacker")
            x = Role::ATTACKER;
        else if (j == "commander")
            x = Role::COMMANDER;
        else if (j == "messenger")
            x = Role::MESSENGER;
        else if (j == "scout")
            x = Role::SCOUT;
        else
        {
            throw std::runtime_error("Input JSON does not conform to schema!");
        }
    }

    /**
     * @brief Role列挙体をJSONへ変換します。
     */
    inline void to_json(json &j, const Role &x)
    {
        switch (x)
        {
        case Role::ATTACKER:
            j = "attacker";
            break;
        case Role::COMMANDER:
            j = "commander";
            break;
        case Role::MESSENGER:
            j = "messenger";
            break;
        case Role::SCOUT:
            j = "scout";
            break;
        default:
            throw std::runtime_error("Unexpected value in enumeration \"Role\": " + std::to_string(static_cast<int>(x)));
        }
    }
}
