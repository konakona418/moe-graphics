#pragma once

#include "Core/Common.hpp"

#include "UI/Common.hpp"
#include "UI/Widget.hpp"

MOE_BEGIN_NAMESPACE

// Layout direction
// Horizontal: Main axis is up-down, cross axis is left-right
// Vertical: Main axis is left-right, cross axis is up-down
enum class BoxLayoutDirection : uint8_t {
    Horizontal,
    Vertical,
};

// Justify: main axis alignment
enum class BoxJustify : uint8_t {
    Start,
    Center,
    End,
    Stretch,
};

// Align: cross axis alignment
enum class BoxAlign : uint8_t {
    Start,
    Center,
    End,
    Stretch,
};

struct BoxWidget : public Widget {
public:
    BoxWidget(BoxLayoutDirection direction = BoxLayoutDirection::Vertical,
              BoxJustify justify = BoxJustify::Start,
              BoxAlign align = BoxAlign::Start)
        : m_direction(direction), m_justify(justify), m_align(align) {
        m_type = WidgetType::Container;
    }

    ~BoxWidget() override = default;

    WidgetType type() const override {
        return WidgetType::Container;
    }

    LayoutSize preferredSize(const LayoutConstraints& constraints) const override;

    void layout(const LayoutRect& rectAssigned) override;

    void setDirection(BoxLayoutDirection direction) {
        m_direction = direction;
    }

    BoxLayoutDirection direction() const {
        return m_direction;
    }

    void setJustify(BoxJustify justify) {
        m_justify = justify;
    }

    BoxJustify justify() const {
        return m_justify;
    }

    void setAlign(BoxAlign align) {
        m_align = align;
    }

    BoxAlign align() const {
        return m_align;
    }

protected:
    BoxLayoutDirection m_direction{BoxLayoutDirection::Vertical};
    BoxJustify m_justify{BoxJustify::Start};
    BoxAlign m_align{BoxAlign::Start};
};

MOE_END_NAMESPACE