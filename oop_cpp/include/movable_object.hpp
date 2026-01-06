#pragma once

#include "sim_object.hpp"

/**
 * @brief 経路に沿って移動するオブジェクトの基底クラスです。
 *
 * @details 共通の移動ロジックを親に置き、斥候・伝令・攻撃の違いは派生クラスで追加します。
 */
class MovableObject : public SimObject {
public:
    /**
     * @brief 移動に必要な経路情報と時間情報を受け取って初期化します。
     */
    MovableObject(std::string id,
                  std::string team_id,
                  simoop::Role role,
                  int start_sec,
                  std::vector<RoutePoint> route,
                  std::vector<std::string> network,
                  std::vector<double> segment_end_secs,
                  double total_duration_sec);

    /**
     * @brief 経路に沿った位置更新を行います。
     */
    void updatePosition(int time_sec) override;

protected:
    std::vector<double> m_segment_end_secs;
    double m_total_duration_sec = 0.0;
};
