#include "Core/Logger.hpp"
#include <mutex>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>


namespace moe {
    std::shared_ptr<Logger> Logger::m_instance{nullptr};

    void Logger::initialize() {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("[%T] [%^%l%$] %v");

        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("engine.log", true);
        file_sink->set_pattern("[%Y-%m-%d %T] [%l] %v");

        std::vector<spdlog::sink_ptr> sinks{console_sink, file_sink};
        m_logger = std::make_shared<spdlog::logger>("moe", sinks.begin(), sinks.end());
        m_logger->set_level(spdlog::level::debug);
        m_logger->flush_on(spdlog::level::info);

        spdlog::register_logger(m_logger);
    }

    void Logger::shutdown() {
        spdlog::drop_all();
        m_logger.reset();
    }

    void Logger::update() {
        if (m_logger) m_logger->flush();
    }

    std::shared_ptr<Logger> Logger::get() {
        static std::once_flag flag;
        std::call_once(flag, []() {
            m_instance = std::make_shared<Logger>();
            m_instance->initialize();
        });
        return m_instance;
    }
}// namespace moe