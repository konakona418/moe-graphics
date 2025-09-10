#include "Core/Window.hpp"
#include "Core/Logger.hpp"

namespace moe {
    void Window::initialize(int width, int height, const std::string& title) {
        Logger::info("Initializing GLFW window...");

        if (!glfwInit()) {
            Logger::critical("Failed to initialize GLFW");
            throw std::runtime_error("GLFW init failed");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        m_window = glfwCreateWindow(width, height, title.c_str(), nullptr, nullptr);

        if (!m_window) {
            Logger::critical("Failed to create GLFW window");
            throw std::runtime_error("GLFW window creation failed");
        }

        glfwSetWindowUserPointer(m_window, this);
        glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
            auto win = reinterpret_cast<Window*>(glfwGetWindowUserPointer(window));
            if (win && win->m_resizeCallback) {
                win->m_resizeCallback(width, height);
            }
        });

        Logger::info("GLFW window created successfully.");
    }
    void Window::shutdown() {
        Logger::info("Shutting down GLFW window...");

        if (m_window) glfwDestroyWindow(m_window);
        glfwTerminate();

        Logger::info("GLFW window shut down successfully.");
    }
}// namespace moe