#pragma once
#include <memory>

#include "Core/Logger.hpp"
#include "Core/Window.hpp"

namespace moe {
    class App {
    public:
        void initialize();

        void runUntilExit();

        void shutdown();

    private:
        std::shared_ptr<Logger> m_logger;
        std::unique_ptr<Window> m_window;
    };
}// namespace moe