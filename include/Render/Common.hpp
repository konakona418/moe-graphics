#pragma once

#include "Core/Common.hpp"
#include "Math/Transform.hpp"

namespace moe {
    namespace Constants {
        constexpr uint32_t FRAMES_IN_FLIGHT = 2;
    }

    using RenderableId = uint32_t;

    struct RenderCommand {
        RenderableId renderableId;
        Transform transform;
    };
}// namespace moe