#pragma once

#include "Core/Common.hpp"

namespace moe {
    struct WindowEvent {
        struct Close {};
        struct Minimize {};
        struct RestoreFromMinimize {};
        struct Resize {
            uint32_t width;
            uint32_t height;
        };

        // ! keydown keyup is not safe. some events will randomly be lost
        struct KeyDown {
            uint32_t keyCode;
        };

        struct KeyRepeat {
            uint32_t keyCode;
        };

        struct KeyUp {
            uint32_t keyCode;
        };

        struct MouseMove {
            float deltaX;
            float deltaY;
        };

        Variant<std::monostate,
                Close,
                Minimize,
                RestoreFromMinimize,
                Resize,
                KeyDown,
                KeyRepeat,
                KeyUp,
                MouseMove>
                args;

        template<typename T>
        bool is() { return std::holds_alternative<T>(args); }

        template<typename T>
        auto getIf() {
            if (std::holds_alternative<T>(args)) {
                return Optional<T>{std::get<T>(args)};
            }
            return Optional<T>{std::nullopt};
        }
    };
}// namespace moe