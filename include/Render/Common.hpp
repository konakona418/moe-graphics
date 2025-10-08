#pragma once

#include "Core/Common.hpp"
#include "Math/Transform.hpp"

namespace moe {
    namespace Constants {
        constexpr uint32_t FRAMES_IN_FLIGHT = 2;
    }

    using BaseIdType = uint32_t;

    using RenderableId = BaseIdType;
    using AnimationId = BaseIdType;
    using ComputeSkinHandleId = BaseIdType;

    constexpr RenderableId NULL_RENDERABLE_ID = std::numeric_limits<RenderableId>::max();
    constexpr AnimationId NULL_ANIMATION_ID = std::numeric_limits<AnimationId>::max();
    constexpr ComputeSkinHandleId NULL_COMPUTE_SKIN_HANDLE_ID = std::numeric_limits<ComputeSkinHandleId>::max();

    struct RenderCommand {
        RenderableId renderableId;
        Transform transform;

        ComputeSkinHandleId computeHandle{NULL_COMPUTE_SKIN_HANDLE_ID};
    };

    struct ComputeSkinCommand {
        RenderableId renderableId;

        AnimationId animationId;
        float time;
    };
}// namespace moe