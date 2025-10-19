#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanRenderable.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

#include "Core/ResourceCache.hpp"

#include "Math/Color.hpp"


namespace moe {
    struct VulkanEngine;

    struct VulkanRenderTarget {
        Vector<VulkanRenderPacket> renderPackets;

        void resetDynamicState() {
            renderPackets.clear();
        }
    };

    struct VulkanRenderTargetLoaderFunctor {
        SharedResource<VulkanRenderTarget> operator()(VulkanRenderTarget&& target) {
            return std::make_shared<VulkanRenderTarget>(std::forward<VulkanRenderTarget>(target));
        }
    };

    struct VulkanRenderTargetCache
        : public ResourceCache<
                  RenderTargetId,
                  VulkanRenderTarget,
                  VulkanRenderTargetLoaderFunctor> {};

    struct VulkanRenderView {
        enum class BlendMode {
            None,
            AlphaBlend,
            // todo: add more
        };

        using Priority = uint32_t;
        static constexpr Priority PRIORITY_LOWEST = std::numeric_limits<Priority>::max();
        static constexpr Priority PRIORITY_HIGHEST = 0;
        static constexpr Priority PRIORITY_DEFAULT = PRIORITY_LOWEST / 2;

        VulkanEngine* engine{nullptr};

        RenderTargetId targetId{NULL_RENDER_TARGET_ID};

        ImageId drawImageId{NULL_IMAGE_ID};
        ImageId depthImageId{NULL_IMAGE_ID};
        ImageId msaaResolveImageId{NULL_IMAGE_ID};// only used if multisampling is enabled

        Color clearColor{0.f, 0.f, 0.f, 1.f};
        BlendMode blendMode{BlendMode::None};

        // Camera provider must outlive the render view
        VPMatrixProvider* cameraProvider{nullptr};
        Viewport viewport{};

        Priority priority{PRIORITY_DEFAULT};
        bool isActive{true};

        void init(
                VulkanEngine* engine,
                RenderTargetId targetId,
                VPMatrixProvider* cameraProvider,
                Viewport viewport, Color clearColor = {0.f, 0.f, 0.f, 1.f},
                BlendMode blendMode = BlendMode::None,
                Priority priority = PRIORITY_DEFAULT);

        void cleanup();

        VulkanRenderView() = default;
        ~VulkanRenderView() = default;
    };

    struct VulkanRenderViewLoaderFunctor {
        SharedResource<VulkanRenderView> operator()(VulkanRenderView&& view) {
            return std::make_shared<VulkanRenderView>(std::forward<VulkanRenderView>(view));
        }
    };

    struct VulkanRenderViewCache
        : public ResourceCache<
                  RenderViewId,
                  VulkanRenderView,
                  VulkanRenderViewLoaderFunctor> {};

}// namespace moe