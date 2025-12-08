#include "UI/SpriteWidget.hpp"

MOE_BEGIN_NAMESPACE

LayoutSize SpriteWidget::preferredSize(const LayoutConstraints& constraints) const {
    float totalWidth =
            m_spriteSize.width +
            m_margin.left + m_margin.right +
            m_padding.left + m_padding.right;

    float totalHeight =
            m_spriteSize.height +
            m_margin.top + m_margin.bottom +
            m_padding.top + m_padding.bottom;

    totalWidth = Math::clamp(
            totalWidth,
            constraints.minWidth,
            constraints.maxWidth);
    totalHeight = Math::clamp(
            totalHeight,
            constraints.minHeight,
            constraints.maxHeight);

    return LayoutSize{totalWidth, totalHeight};
}

void SpriteWidget::layout(const LayoutRect& rectAssigned) {
    m_calculatedBounds = rectAssigned;

    LayoutRect contentRect;
    contentRect.x = rectAssigned.x + m_padding.left;
    contentRect.y = rectAssigned.y + m_padding.top;
    contentRect.width = rectAssigned.width - (m_padding.left + m_padding.right);
    contentRect.height = rectAssigned.height - (m_padding.top + m_padding.bottom);

    const auto& spriteSize = m_spriteSize;
    float scaleX = contentRect.width / spriteSize.width;
    float scaleY = contentRect.height / spriteSize.height;

    scaleX = Math::min(scaleX, 1.f);
    scaleY = Math::min(scaleY, 1.f);

    float finalWidth = spriteSize.width * scaleX;
    float finalHeight = spriteSize.height * scaleY;

    float offsetX = (contentRect.width - finalWidth) * 0.5f;
    float offsetY = (contentRect.height - finalHeight) * 0.5f;

    m_spriteRenderRect = {
            .x = contentRect.x + offsetX,
            .y = contentRect.y + offsetY,
            .width = finalWidth,
            .height = finalHeight,
    };
}

MOE_END_NAMESPACE