#include "Core/Config.hpp"
#include <mutex>

namespace moe {
    std::shared_ptr<Config> Config::m_instance{nullptr};

    std::shared_ptr<Config> Config::get() {
        static std::once_flag flag;
        std::call_once(flag, [&]() { m_instance = std::make_shared<Config>(); });

        return m_instance;
    }
}// namespace moe