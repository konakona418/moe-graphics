#pragma once

#include "Render/Vulkan/VulkanContext.hpp"
#include "Render/Vulkan/VulkanRenderTarget.hpp"
#include "Render/Vulkan/VulkanSwapchain.hpp"

namespace moe {
    class VulkanSwapchainRenderTarget : public VulkanRenderTarget {
    public:
        VulkanSwapchainRenderTarget() = default;
        ~VulkanSwapchainRenderTarget() = default;

        void initialize(VulkanContext* ctx, std::pair<int, int> extent);
        void shutdown();

        void recreate(std::pair<int, int> newExtent);

        VkExtent2D getExtent() override;

        VkFramebuffer getFramebuffer() override;

        VkRenderPass getRenderPass() override;

    private:
        VulkanContext* m_context{nullptr};

        VulkanSwapchain m_vkSwapchain;

        VkRenderPass m_renderPass;
        std::vector<VkFramebuffer> m_framebuffers;

        void createRenderPass();

        void createFramebuffers();
    };
}// namespace moe