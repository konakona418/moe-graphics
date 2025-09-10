#include "Core/Logger.hpp"

#include "Render/Vulkan/VulkanSwapchain.hpp"

namespace moe {

    void VulkanSwapchain::initialize(VulkanContext* context, std::pair<int, int> extent) {
        Logger::info("Creating swapchain...");

        m_context = context;
        createSwapchain(extent);
        getSwapchainImages();
        createSwapchainImageViews();

        Logger::info("Swapchain created successfully.");
    }

    void VulkanSwapchain::shutdown() {
        cleanup();
    }

    void VulkanSwapchain::cleanup() {
        m_swapchain.destroy_image_views(m_swapchainImageViews);
        m_swapchainImageViews.clear();

        vkb::destroy_swapchain(m_swapchain);
    }

    void VulkanSwapchain::recreate(std::pair<int, int> newExtent) {
        vkDeviceWaitIdle(m_context->m_device);

        cleanup();

        createSwapchain(newExtent);
        getSwapchainImages();
        createSwapchainImageViews();
    }

    void VulkanSwapchain::createSwapchain(std::pair<int, int> extent) {
        vkb::SwapchainBuilder swapchainBuilder{m_context->m_device};

        /*if (!m_swapchain) {
            swapchainBuilder.set_old_swapchain(VK_NULL_HANDLE);
        } else {
            swapchainBuilder.set_old_swapchain(m_swapchain);
        }*/

        auto vkbSwapchain =
                swapchainBuilder
                        .set_desired_extent(extent.first, extent.second)
                        .set_desired_min_image_count(VulkanContext::MAX_FRAMES_IN_FLIGHT)
                        .build();

        if (!vkbSwapchain) {
            Logger::critical("Failed to create swapchain: {}", vkbSwapchain.error().message());
            throw std::runtime_error("Failed to create swapchain");
        }

        if (vkbSwapchain->image_count != VulkanContext::MAX_FRAMES_IN_FLIGHT) {
            Logger::warn("Swapchain image count {} does not match MAX_FRAMES_IN_FLIGHT {}",
                         vkbSwapchain->image_count, VulkanContext::MAX_FRAMES_IN_FLIGHT);
        }

        Logger::info("Swapchain created with {} images, extent: {}x{}",
                     vkbSwapchain->image_count,
                     vkbSwapchain->extent.width,
                     vkbSwapchain->extent.height);

        m_swapchain = *vkbSwapchain;
    }

    void VulkanSwapchain::getSwapchainImages() {
        auto images = m_swapchain.get_images();

        if (!images) {
            Logger::critical("Failed to get swapchain images: {}", images.error().message());
            throw std::runtime_error("Failed to get swapchain images");
        }

        m_swapchainImages = std::move(images.value());
    }

    void VulkanSwapchain::createSwapchainImageViews() {
        auto imageViews = m_swapchain.get_image_views();

        if (!imageViews) {
            Logger::critical("Failed to get swapchain image views: {}", imageViews.error().message());
            throw std::runtime_error("Failed to get swapchain image views");
        }

        m_swapchainImageViews = std::move(imageViews.value());
    }

}// namespace moe