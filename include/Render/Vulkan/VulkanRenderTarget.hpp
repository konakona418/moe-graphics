#pragma once

#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanRenderable.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

#include "Core/ResourceCache.hpp"

#include "Math/Color.hpp"


namespace moe {
    struct VulkanEngine;

    struct VulkanRenderTargetContext {
        Vector<VulkanRenderPacket> renderPackets;

        void resetDynamicState() {
            renderPackets.clear();
        }
    };

    struct VulkanRenderTargetContextLoaderFunctor {
        SharedResource<VulkanRenderTargetContext> operator()(VulkanRenderTargetContext&& context) {
            return std::make_shared<VulkanRenderTargetContext>(std::forward<VulkanRenderTargetContext>(context));
        }
    };

    struct VulkanRenderTargetContextCache
        : public ResourceCache<
                  RenderTargetId,
                  VulkanRenderTargetContext,
                  VulkanRenderTargetContextLoaderFunctor> {};

    struct VulkanRenderTarget {
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

        RenderTargetContextId contextId{NULL_RENDER_TARGET_CONTEXT_ID};

        VulkanAllocatedImage drawImage{};
        VulkanAllocatedImage depthImage{};
        VulkanAllocatedImage msaaResolveImage{};// only used if multisampling is enabled

        Color clearColor{0.f, 0.f, 0.f, 1.f};
        BlendMode blendMode{BlendMode::None};

        SharedPtr<VPMatrixProvider> cameraProvider{nullptr};
        Viewport viewport{};

        Priority priority{PRIORITY_DEFAULT};
        bool isActive{true};

        void init(
                VulkanEngine* engine,
                RenderTargetContextId contextId,
                SharedPtr<VPMatrixProvider> cameraProvider,
                Viewport viewport, Color clearColor = {0.f, 0.f, 0.f, 1.f},
                BlendMode blendMode = BlendMode::None,
                Priority priority = PRIORITY_DEFAULT);

        void cleanup();

        VulkanRenderTarget() = default;
        ~VulkanRenderTarget() = default;
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

}// namespace moe