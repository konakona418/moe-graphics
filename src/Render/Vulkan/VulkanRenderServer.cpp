#include <VkBootstrap.h>
#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>

#include "Render/Vulkan/VulkanContext.hpp"
#include "Render/Vulkan/VulkanRenderServer.hpp"

namespace moe {

    void VulkanRenderServer::initialize() {
        m_context = std::make_unique<VulkanContext>();
        m_context->initialize();
    }

    void VulkanRenderServer::shutdown() {
        m_context->shutdown();
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

}// namespace moe