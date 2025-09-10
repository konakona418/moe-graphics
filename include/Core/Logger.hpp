#pragma once
#include "Module.hpp"
#include <memory>
#include <spdlog/logger.h>
#include <spdlog/spdlog.h>

namespace moe {
    class Logger : public Module {
    public:
        void initialize() override;
        void shutdown() override;
        void flush();

        template<typename... Args>
        static void info(const char* fmt, const Args&... args) {
            get()->log(spdlog::level::info, fmt, args...);
        }
        template<typename... Args>
        static void warn(const char* fmt, const Args&... args) {
            get()->log(spdlog::level::warn, fmt, args...);
        }
        template<typename... Args>
        static void error(const char* fmt, const Args&... args) {
            get()->log(spdlog::level::err, fmt, args...);
        }
        template<typename... Args>
        static void critical(const char* fmt, const Args&... args) {
            get()->log(spdlog::level::critical, fmt, args...);
        }
        template<typename... Args>
        static void debug(const char* fmt, const Args&... args) {
            get()->log(spdlog::level::debug, fmt, args...);
        }

        static std::shared_ptr<Logger> get();

    private:
        template<typename... Args>
        void log(spdlog::level::level_enum lvl, const char* fmt, const Args&... args) {
            if (m_logger) m_logger->log(lvl, fmt, args...);
        }

        std::shared_ptr<spdlog::logger> m_logger;
        static std::shared_ptr<Logger> m_instance;
    };
}// namespace moe