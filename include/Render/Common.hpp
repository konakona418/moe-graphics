#pragma once

#include "Core/Common.hpp"
#include "Math/Transform.hpp"

namespace moe {
    using RenderableId = uint32_t;

    struct RenderCommand {
        RenderableId renderableId;
        Transform transform;
    };
}// namespace moe