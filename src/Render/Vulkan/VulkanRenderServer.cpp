#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "Core/Logger.hpp"

#include "Render/Vulkan/VulkanContext.hpp"
#include "Render/Vulkan/VulkanRenderServer.hpp"

namespace moe {

    void VulkanRenderServer::initialize(VulkanContext* ctx) {
        Logger::info("Starting Vulkan render server...");

        m_context = ctx;
        createSyncObjects();

        Logger::info("Vulkan render server started.");
    }

    void VulkanRenderServer::shutdown() {
        Logger::info("Shutting down Vulkan render server...");

        cleanup();
        m_context = nullptr;

        Logger::info("Vulkan render server shut down successfully.");
    }

    void VulkanRenderServer::cleanup() {
        for (auto fence: m_inFlightFences) {
            vkDestroyFence(m_context->m_device, fence, nullptr);
        }
        m_inFlightFences.clear();

        for (auto fence: m_imagesInFlight) {
            vkDestroyFence(m_context->m_device, fence, nullptr);
        }
        m_imagesInFlight.clear();

        for (auto renderFinishedSemaphore: m_renderFinishedSemaphores) {
            vkDestroySemaphore(m_context->m_device, renderFinishedSemaphore, nullptr);
        }
        m_renderFinishedSemaphores.clear();

        for (auto imageAvailableSemaphore: m_imageAvailableSemaphores) {
            vkDestroySemaphore(m_context->m_device, imageAvailableSemaphore, nullptr);
        }
        m_imageAvailableSemaphores.clear();
    }

    void VulkanRenderServer::beginFrame() {
        // Begin frame rendering
    }

    void VulkanRenderServer::drawMesh() {
        // Issue draw calls for a mesh
    }

    void VulkanRenderServer::endFrame() {
        // End frame rendering and present
    }
    void VulkanRenderServer::createSyncObjects() {
        Logger::info("Creating synchronization objects for frames...");

        auto realSize = VulkanContext::MAX_FRAMES_IN_FLIGHT;

        m_inFlightFences.resize(realSize, VK_NULL_HANDLE);
        m_imagesInFlight.resize(realSize, VK_NULL_HANDLE);

        m_imageAvailableSemaphores.resize(realSize, VK_NULL_HANDLE);
        m_renderFinishedSemaphores.resize(realSize, VK_NULL_HANDLE);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

        for (size_t i = 0; i < realSize; ++i) {
            if (vkCreateSemaphore(m_context->m_device, &semaphoreInfo,
                                  nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_context->m_device, &semaphoreInfo,
                                  nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS) {
                Logger::critical("failed to create semaphores for a frame!");
            }
        }

        for (size_t i = 0; i < realSize; ++i) {
            if (vkCreateFence(m_context->m_device, &fenceInfo,
                              nullptr, &m_inFlightFences[i]) != VK_SUCCESS ||
                vkCreateFence(m_context->m_device, &fenceInfo,
                              nullptr, &m_imagesInFlight[i]) != VK_SUCCESS) {
                Logger::critical("failed to create fence for a frame!");
            }
        }

        Logger::info("Synchronization objects for frames created successfully.");
    }

}// namespace moe