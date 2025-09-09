#pragma once

#include <memory>
#include <string>

namespace moe {
    class Config {
    public:
        static std::shared_ptr<Config> get();

        std::pair<int, int> m_windowSize{1280, 720};
        std::string m_windowTitle{"Moe Graphics Engine"};

        static std::shared_ptr<Config> m_instance;
    };
}// namespace moe