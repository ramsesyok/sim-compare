#pragma once

#include "sim_object.hpp"

// 移動しない固定オブジェクトの基本クラスです。
class FixedObject : public SimObject {
public:
    using SimObject::SimObject;

    void updatePosition(int time_sec) override;
};
