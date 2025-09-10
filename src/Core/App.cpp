#include "Core/App.hpp"

#include "Core/Config.hpp"
#include "Core/Window.hpp"

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

            // todo: handle resize event
        });

        std::pair<int, int> defaultExtent;
        glfwGetFramebufferSize(m_window->getHandle(),
                               &defaultExtent.first, &defaultExtent.second);

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

        m_window->shutdown();

        Logger::info("Goodbye.");
    }
}// namespace moe