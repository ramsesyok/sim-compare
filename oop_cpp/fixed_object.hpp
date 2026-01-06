#pragma once

#include "sim_object.hpp"

// 移動しない固定オブジェクトの基本クラスです。
// SimObjectの振る舞いのうち「位置更新」を固定化することで、役割ごとの差を継承で表現します。
class FixedObject : public SimObject {
public:
    using SimObject::SimObject;

    void updatePosition(int time_sec) override;
};
