#include "UI/BoxWidget.hpp"

MOE_BEGIN_NAMESPACE

LayoutSize BoxWidget::preferredSize(const LayoutConstraints& constraints) const {
    float totalHorizDecoration = m_padding.left + m_padding.right;
    float totalVertDecoration = m_padding.top + m_padding.bottom;

    LayoutConstraints childConstraints;
    childConstraints.minWidth = 0.f;
    childConstraints.minHeight = 0.f;
    childConstraints.maxWidth = Math::max(0.f, constraints.maxWidth - totalHorizDecoration);
    childConstraints.maxHeight = Math::max(0.f, constraints.maxHeight - totalVertDecoration);

    LayoutSize totalSize;
    for (const auto& child: m_children) {
        LayoutSize childSize = child->preferredSize(childConstraints);
        auto& margin = child->margin();

        float childMainAxisMargin = 0.f;
        float childCrossAxisMargin = 0.f;

        if (m_direction == BoxLayoutDirection::Vertical) {
            childMainAxisMargin = margin.top + margin.bottom;
            childCrossAxisMargin = margin.left + margin.right;

            totalSize.width = Math::max(totalSize.width, childSize.width + childCrossAxisMargin);
            totalSize.height += childSize.height + childMainAxisMargin;
        } else {
            childMainAxisMargin = margin.left + margin.right;
            childCrossAxisMargin = margin.top + margin.bottom;

            totalSize.width += childSize.width + childMainAxisMargin;
            totalSize.height = Math::max(totalSize.height, childSize.height + childCrossAxisMargin);
        }
    }
    totalSize.width += m_padding.left + m_padding.right;
    totalSize.height += m_padding.top + m_padding.bottom;

    return clampConstraintMinMax(totalSize, constraints);
}

void BoxWidget::layout(const LayoutRect& rectAssigned) {
    // todo: WIP
}

MOE_END_NAMESPACE