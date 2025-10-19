#include "Render/Vulkan/VulkanRenderTarget.hpp"

#include "Render/Vulkan/VulkanCamera.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"
#include "Render/Vulkan/VulkanInitializers.hpp"
#include "Render/Vulkan/VulkanRenderable.hpp"

namespace moe {
    void VulkanRenderView::init(
            VulkanEngine* engine,
            RenderTargetId targetId,
            VPMatrixProvider* cameraProvider,
            Viewport viewport, Color clearColor,
            BlendMode blendMode,
            Priority priority) {
        MOE_ASSERT(engine != nullptr, "VulkanEngine is null");
        MOE_ASSERT(cameraProvider != nullptr, "cameraProvider is null");

        this->engine = engine;
        this->targetId = targetId;
        this->cameraProvider = std::move(cameraProvider);
        this->viewport = viewport;
        this->clearColor = clearColor;
        this->blendMode = blendMode;
        this->priority = priority;

        VkExtent3D drawExtent = {
                static_cast<uint32_t>(viewport.width),
                static_cast<uint32_t>(viewport.height),
                1};

        VulkanAllocatedImage drawImage, depthImage, msaaResolveImage;

        drawImage.imageFormat = VK_FORMAT_R16G16B16A16_SFLOAT;
        drawImage.imageExtent = drawExtent;

        depthImage.imageFormat = VK_FORMAT_D32_SFLOAT;
        depthImage.imageExtent = drawExtent;

        VkImageUsageFlags drawImageUsage{};
        drawImageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        drawImageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        drawImageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        drawImageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;
        drawImageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;

        VkImageCreateInfo drawImageInfo = VkInit::imageCreateInfo(drawImage.imageFormat, drawImageUsage, drawExtent);
        drawImageInfo.samples = engine->isMultisamplingEnabled() ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;

        drawImage = engine->allocateImage(drawImageInfo);

        VkImageUsageFlags depthImageUsage{};
        depthImageUsage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        depthImageUsage |= VK_IMAGE_USAGE_SAMPLED_BIT;

        VkImageCreateInfo depthImageInfo = VkInit::imageCreateInfo(depthImage.imageFormat, depthImageUsage, drawExtent);
        depthImageInfo.samples = engine->isMultisamplingEnabled() ? VK_SAMPLE_COUNT_4_BIT : VK_SAMPLE_COUNT_1_BIT;

        depthImage = engine->allocateImage(depthImageInfo);

        if (engine->isMultisamplingEnabled()) {
            VkImageUsageFlags msaaResolveImageUsage{};
            msaaResolveImageUsage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
            msaaResolveImageUsage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
            msaaResolveImageUsage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            msaaResolveImageUsage |= VK_IMAGE_USAGE_STORAGE_BIT;

            VkImageCreateInfo msaaResolveImageInfo = VkInit::imageCreateInfo(msaaResolveImage.imageFormat, msaaResolveImageUsage, drawExtent);
            msaaResolveImageInfo.samples = VK_SAMPLE_COUNT_1_BIT;

            msaaResolveImage = engine->allocateImage(msaaResolveImageInfo);
        }

        drawImageId = engine->m_caches.imageCache.addImage(std::move(drawImage));
        depthImageId = engine->m_caches.imageCache.addImage(std::move(depthImage));
        if (engine->isMultisamplingEnabled()) {
            msaaResolveImageId = engine->m_caches.imageCache.addImage(std::move(msaaResolveImage));
        }
    }

    void VulkanRenderView::cleanup() {
        engine = nullptr;
    }
}// namespace moe