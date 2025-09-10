#include "Core/App.hpp"

#include "Core/Config.hpp"
#include "Core/Window.hpp"

#include "Render/Vulkan/VulkanContext.hpp"

namespace moe {
    void App::initialize() {
        Logger::info("Starting Moe Graphics Engine application...");

        const auto& windowMetrics = Config::get()->m_windowSize;
        const auto& windowTitle = Config::get()->m_windowTitle;

        Logger::info("Creating application window ({}x{}): {}",
                     windowMetrics.first, windowMetrics.second, windowTitle);

        m_window = std::make_unique<Window>();
        m_window->initialize(windowMetrics.first, windowMetrics.second, windowTitle);
        m_window->setResizeCallback([this](int width, int height) {
            Logger::info("Window resized to {}x{}", width, height);

            std::pair<int, int> newExtent;
            glfwGetFramebufferSize(this->m_window->getHandle(), &newExtent.first, &newExtent.second);
            this->m_defaultRenderTarget->recreate(newExtent);
        });

        std::pair<int, int> defaultExtent;
        glfwGetFramebufferSize(m_window->getHandle(),
                               &defaultExtent.first, &defaultExtent.second);

        m_vulkanContext = std::make_unique<VulkanContext>();
        m_vulkanContext->initialize(m_window->getHandle());

        m_renderServer = std::make_unique<VulkanRenderServer>();
        m_renderServer->initialize(m_vulkanContext.get());

        m_defaultRenderTarget = std::make_unique<VulkanSwapchainRenderTarget>();
        m_defaultRenderTarget->initialize(m_vulkanContext.get(), defaultExtent);

        Logger::info("Moe Graphics Engine application started.");
    }

    void App::runUntilExit() {
        Logger::info("Launching main render loop...");

        while (!m_window->shouldClose()) {
            glfwPollEvents();
        }
    }

    void App::shutdown() {
        Logger::info("Shutting down engine...");

        m_defaultRenderTarget->shutdown();
        m_renderServer->shutdown();
        m_vulkanContext->shutdown();

        m_window->shutdown();

        Logger::info("Goodbye.");
    }
}// namespace moe