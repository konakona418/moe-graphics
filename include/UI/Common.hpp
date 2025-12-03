#pragma once

#include "Core/Common.hpp"
#include "Math/Util.hpp"

MOE_BEGIN_NAMESPACE

enum class WidgetType : uint8_t {
    Unknown,
    Container,
    Text,
    Button,
    Image,
};

struct LayoutConstraints {
    float minWidth{0.f};
    float maxWidth{std::numeric_limits<float>::max()};
    float minHeight{0.f};
    float maxHeight{std::numeric_limits<float>::max()};
};

struct LayoutSize {
    float width{0.f};
    float height{0.f};
};

struct LayourPos {
    float x{0.f};
    float y{0.f};
};

struct LayoutRect {
    float x{0.f};
    float y{0.f};
    float width{0.f};
    float height{0.f};
};

struct LayoutBorders {
    float left{0.f};
    float top{0.f};
    float right{0.f};
    float bottom{0.f};
};

inline LayoutSize clampConstraintMinMax(const LayoutSize& size, const LayoutConstraints& constraints) {
    LayoutSize clampedSize;
    clampedSize.width = Math::clamp(
            size.width,
            constraints.minWidth,
            constraints.maxWidth);
    clampedSize.height = Math::clamp(
            size.height,
            constraints.minHeight,
            constraints.maxHeight);
    return clampedSize;
}

MOE_END_NAMESPACE