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

    using RenderTargetId = BaseIdType;
    using RenderTargetContextId = BaseIdType;

    constexpr RenderableId NULL_RENDERABLE_ID = std::numeric_limits<RenderableId>::max();
    constexpr AnimationId NULL_ANIMATION_ID = std::numeric_limits<AnimationId>::max();
    constexpr ComputeSkinHandleId NULL_COMPUTE_SKIN_HANDLE_ID = std::numeric_limits<ComputeSkinHandleId>::max();

    constexpr RenderTargetId NULL_RENDER_TARGET_ID = std::numeric_limits<RenderTargetId>::max();
    constexpr RenderTargetContextId NULL_RENDER_TARGET_CONTEXT_ID = std::numeric_limits<RenderTargetContextId>::max();

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

    struct VPMatrixProvider {
        virtual glm::mat4 viewMatrix() const = 0;
        virtual glm::mat4 projectionMatrix(float aspectRatio) const = 0;
    };

    struct Viewport {
        int32_t x{0};
        int32_t y{0};
        uint32_t width{0};
        uint32_t height{0};
    };
}// namespace moe