#include "Core/App.hpp"

namespace moe {
    void App::initialize() {
        Logger::info("Starting Moe Graphics Engine application...");

        Logger::info("Moe Graphics Engine application started.");
    }

    void App::runUntilExit() {
        Logger::info("Launching main render loop...");
    }

    void App::shutdown() {
        Logger::info("Shutting down engine...");

        Logger::info("Goodbye.");
    }
}// namespace moe