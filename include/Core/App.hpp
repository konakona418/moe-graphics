#pragma once
#include <memory>

#include "Core/Logger.hpp"

namespace moe {
    class App {
    public:
        void initialize();

        void runUntilExit();

        void shutdown();

    private:
        std::shared_ptr<Logger> m_logger;
    };
}// namespace moe