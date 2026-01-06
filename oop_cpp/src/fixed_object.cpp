#include "fixed_object.hpp"

void FixedObject::updatePosition(int /*time_sec*/) {
    // 固定オブジェクトは移動しないため、常に最初の経路点に据え置きます。
    if (!m_route.empty()) {
        m_position = m_route.front().ecef;
    }
}
