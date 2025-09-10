#pragma once
#include <memory>

#include "Core/Logger.hpp"
#include "Core/Window.hpp"

#include "Render/Vulkan/VulkanContext.hpp"
#include "Render/Vulkan/VulkanRenderServer.hpp"
#include "Render/Vulkan/VulkanSwapchainRenderTarget.hpp"

namespace moe {
    class App {
    public:
        void initialize();

        void runUntilExit();

        void shutdown();

    private:
        std::shared_ptr<Logger> m_logger;
        std::unique_ptr<Window> m_window;
        std::unique_ptr<VulkanContext> m_vulkanContext;

        // only vulkan render server supported for now
        std::unique_ptr<VulkanRenderServer> m_renderServer;
        std::unique_ptr<VulkanSwapchainRenderTarget> m_defaultRenderTarget;
    };
}// namespace moe