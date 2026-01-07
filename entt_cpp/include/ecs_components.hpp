#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "geo.hpp"
#include "jsonobj/scenario.hpp"
#include "route.hpp"

/**
 * @brief 探知した相手の情報を保持し、失探判定に利用するための構造体です。
 *
 * @details 斥候ごとに「前の時刻で探知していた相手」を覚えるために使います。
 *          これにより、今は見えていない相手を「失探」として記録できるようになります。
 */
struct DetectionInfo {
    double lat_deg = 0.0;
    double lon_deg = 0.0;
    double alt_m = 0.0;
    int distance_m = 0;
};

/**
 * @brief エンティティに「オブジェクトID」を与えるためのコンポーネントです。
 *
 * @details ECSでは「必要な属性だけを必要なエンティティに付与する」ため、
 *          文字列のIDも専用の構造体として分離しておきます。
 */
struct ObjectIdComponent {
    std::string value;
};

/**
 * @brief チームIDを表すコンポーネントです。
 *
 * @details 同じチーム同士は探知対象から除外するため、必須の識別子として保持します。
 */
struct TeamIdComponent {
    std::string value;
};

/**
 * @brief 役割(司令・斥候・伝令・攻撃)を表すコンポーネントです。
 *
 * @details 役割によって更新やイベント発火の条件が変わるため、分岐のキーになります。
 */
struct RoleComponent {
    jsonobj::Role value = jsonobj::Role::COMMANDER;
};

/**
 * @brief シナリオで指定された開始時刻を保持するコンポーネントです。
 *
 * @details 開始前のオブジェクトは初期位置に留めるため、この値が必要になります。
 */
struct StartSecComponent {
    int value = 0;
};

/**
 * @brief ECEF座標の現在位置を保持するコンポーネントです。
 *
 * @details ECSでは位置更新だけを行うシステムが多いため、位置は独立した構造体にします。
 */
struct PositionComponent {
    Ecef ecef{0.0, 0.0, 0.0};
};

/**
 * @brief 経路点と区間時間をまとめて保持するコンポーネントです。
 *
 * @details ルートと移動時間はシミュレーション中に参照し続けるため、
 *          1つのコンポーネントにまとめてキャッシュし、計算の再実行を避けます。
 */
struct RouteComponent {
    std::vector<RoutePoint> points;
    std::vector<double> segment_end_secs;
    double total_duration = 0.0;
};

/**
 * @brief 探知状態(前回見えていた対象)を保持するコンポーネントです。
 *
 * @details 斥候だけが持つコンポーネントで、失探イベントの判定に使います。
 */
struct DetectionStateComponent {
    std::unordered_map<std::string, DetectionInfo> detected;
};

/**
 * @brief 爆破イベントの発火済み状態を保持するコンポーネントです。
 *
 * @details 攻撃役は1回だけ爆破するため、再発火防止のフラグを明示的に持たせます。
 */
struct DetonationStateComponent {
    bool has_detonated = false;
};
