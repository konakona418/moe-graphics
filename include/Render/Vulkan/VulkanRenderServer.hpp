#pragma once

#include "VulkanContext.hpp"

namespace moe {
    class VulkanRenderServer {
    public:
        VulkanRenderServer() = default;
        ~VulkanRenderServer() = default;

        void initialize(VulkanContext* ctx);

        void shutdown();

        void beginFrame();

        void drawMesh();

        void endFrame();

    private:
        VulkanContext* m_context{nullptr};

        std::vector<VkFence> m_inFlightFences;
        std::vector<VkFence> m_imagesInFlight;
        std::vector<VkSemaphore> m_imageAvailableSemaphores;
        std::vector<VkSemaphore> m_renderFinishedSemaphores;

        void cleanup();

        void createSyncObjects();
    };
}// namespace moe