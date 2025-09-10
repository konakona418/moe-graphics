#include "Render/Vulkan/VulkanSwapchainRenderTarget.hpp"
#include "Core/Logger.hpp"


namespace moe {
    void VulkanSwapchainRenderTarget::initialize(VulkanContext* ctx, std::pair<int, int> extent) {
        Logger::info("Initializing Vulkan swapchain render target...");

        m_context = ctx;
        m_vkSwapchain.initialize(ctx, extent);

        Logger::info("Vulkan swapchain render target initialized successfully.");
    }

    void VulkanSwapchainRenderTarget::shutdown() {
        Logger::info("Shutting down Vulkan swapchain render target...");

        m_vkSwapchain.shutdown();
        m_context = nullptr;

        Logger::info("Vulkan swapchain render target shut down successfully.");
    }

    void VulkanSwapchainRenderTarget::recreate(std::pair<int, int> newExtent) {
        m_vkSwapchain.recreate(newExtent);
    }

    VkExtent2D VulkanSwapchainRenderTarget::getExtent() {
        return m_vkSwapchain.m_swapchain.extent;
    }

    VkFramebuffer VulkanSwapchainRenderTarget::getFramebuffer() {
        // todo
        return VK_NULL_HANDLE;
    }

    VkRenderPass VulkanSwapchainRenderTarget::getRenderPass() {
        return m_renderPass;
    }

    void VulkanSwapchainRenderTarget::createRenderPass() {
        // todo
    }

    void VulkanSwapchainRenderTarget::createFramebuffers() {
        // todo
    }
}// namespace moe
