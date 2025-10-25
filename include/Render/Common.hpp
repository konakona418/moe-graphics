#pragma once

#include "Core/Common.hpp"
#include "Math/Color.hpp"
#include "Math/Transform.hpp"

namespace moe {
    namespace Constants {
        constexpr uint32_t FRAMES_IN_FLIGHT = 2;
    }

    namespace Loader {
        struct GltfT {
            constexpr GltfT() = default;
        };
        static constexpr GltfT Gltf{};
        struct ObjT {
            constexpr ObjT() = default;
        };
        static constexpr ObjT Obj{};

        struct ImageT {
            constexpr ImageT() = default;
        };
        static constexpr ImageT Image{};

        struct FontT {
            constexpr FontT() = default;
        };
        static constexpr FontT Font{};
    }// namespace Loader

    using BaseIdType = uint32_t;

    using RenderableId = BaseIdType;
    using AnimationId = BaseIdType;
    using ComputeSkinHandleId = BaseIdType;
    using ImageId = BaseIdType;

    using RenderTargetId = BaseIdType;
    using RenderViewId = BaseIdType;

    constexpr RenderableId NULL_RENDERABLE_ID = std::numeric_limits<RenderableId>::max();
    constexpr AnimationId NULL_ANIMATION_ID = std::numeric_limits<AnimationId>::max();
    constexpr ComputeSkinHandleId NULL_COMPUTE_SKIN_HANDLE_ID = std::numeric_limits<ComputeSkinHandleId>::max();
    constexpr ImageId NULL_IMAGE_ID = std::numeric_limits<ImageId>::max();

    constexpr RenderTargetId NULL_RENDER_TARGET_ID = std::numeric_limits<RenderTargetId>::max();
    constexpr RenderViewId NULL_RENDER_VIEW_ID = std::numeric_limits<RenderViewId>::max();

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