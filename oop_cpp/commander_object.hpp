#pragma once

#include "fixed_object.hpp"

/**
 * @brief 司令官は固定オブジェクトとして扱い、特別な動作は追加しません。
 *
 * @details 「何もしない」という振る舞いも派生クラスとして明示すると、
 *          役割の違いが理解しやすくなります。
 */
class CommanderObject : public FixedObject {
public:
    using FixedObject::FixedObject;
};
