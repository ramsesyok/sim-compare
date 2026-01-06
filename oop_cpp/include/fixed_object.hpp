#pragma once

#include "sim_object.hpp"

/**
 * @brief 移動しない固定オブジェクトの基本クラスです。
 *
 * @details SimObjectの振る舞いのうち「位置更新」を固定化することで、
 *          役割ごとの差を継承で表現します。
 */
class FixedObject : public SimObject {
public:
    using SimObject::SimObject;

    /**
     * @brief 固定オブジェクトの位置更新を行います。
     */
    void updatePosition(int time_sec) override;
};
