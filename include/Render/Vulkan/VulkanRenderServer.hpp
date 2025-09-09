#pragma once

#include "VulkanContext.hpp"
#include <memory>

namespace moe {
    class VulkanRenderServer {
    public:
        VulkanRenderServer() = default;
        ~VulkanRenderServer() = default;

        void initialize();

        void shutdown();

        void beginFrame();

        void drawMesh();

        void endFrame();

    private:
        std::unique_ptr<VulkanContext> m_context;
    };
}// namespace moe