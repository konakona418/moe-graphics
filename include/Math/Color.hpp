#pragma once

#include "Core/Common.hpp"
#include "Math/Common.hpp"

namespace moe {
    struct Color {
        float r{0.f}, g{0.f}, b{0.f}, a{1.f};

        Color() = default;
        Color(float r, float g, float b, float a = 1.f) : r(r), g(g), b(b), a(a) {}

        static Color fromNormalized(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
            return Color(r / 255.f, g / 255.f, b / 255.f, a / 255.f);
        }

        glm::vec4 toVec4() const {
            return glm::vec4(r, g, b, a);
        }

        glm::vec3 toVec3() const {
            return glm::vec3(r, g, b);
        }
    };
}// namespace moe