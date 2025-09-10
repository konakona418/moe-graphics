#pragma once

#include <vector>

#include <VkBootstrap.h>
#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Render/Vulkan/VulkanContext.hpp"

namespace moe {
    class VulkanSwapchain {
    public:
        friend class VulkanSwapchainRenderTarget;

        VulkanSwapchain() = default;
        ~VulkanSwapchain() = default;

        void initialize(VulkanContext* context, std::pair<int, int> extent);

        void shutdown();

        void recreate(std::pair<int, int> newExtent);

    private:
        VulkanContext* m_context{nullptr};

        vkb::Swapchain m_swapchain{};
        std::vector<VkImage> m_swapchainImages;
        std::vector<VkImageView> m_swapchainImageViews;

        void cleanup();

        void createSwapchain(std::pair<int, int> extent);

        void getSwapchainImages();

        void createSwapchainImageViews();
    };
}// namespace moe