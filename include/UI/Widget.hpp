#pragma once

#include "UI/Common.hpp"

#include "Core/Common.hpp"
#include "Core/RefCounted.hpp"

MOE_BEGIN_NAMESPACE

struct Widget : public RefCounted<Widget> {
public:
    Widget() = default;
    virtual ~Widget() = default;

    virtual WidgetType type() const {
        return m_type;
    }

    virtual LayoutSize preferredSize(const LayoutConstraints& constraints) const = 0;

    virtual void layout(const LayoutRect& rectAssigned) = 0;

    void addChild(Ref<Widget>& child) {
        m_children.push_back(child);
        child->m_parent = this->intoRef();
    }

    const Vector<Ref<Widget>>& children() const {
        return m_children;
    }

    void setMargin(const LayoutRect& margin) {
        m_margin = margin;
    }

    const LayoutRect& margin() const {
        return m_margin;
    }

    void setPadding(const LayoutRect& padding) {
        m_padding = padding;
    }

    const LayoutRect& padding() const {
        return m_padding;
    }

    const LayoutRect& calculatedBounds() const {
        return m_calculatedBounds;
    }

protected:
    Ref<Widget> m_parent{nullptr};
    Vector<Ref<Widget>> m_children;

    LayoutRect m_margin;
    LayoutRect m_padding;

    LayoutRect m_calculatedBounds;

    WidgetType m_type{WidgetType::Unknown};
};

MOE_END_NAMESPACE