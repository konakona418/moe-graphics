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
    LayoutRect contentRect;
    contentRect.x = rectAssigned.x + m_padding.left;
    contentRect.y = rectAssigned.y + m_padding.top;
    contentRect.width = rectAssigned.width - (m_padding.left + m_padding.right);
    contentRect.height = rectAssigned.height - (m_padding.top + m_padding.bottom);

    float mainAxisSize = (m_direction == BoxLayoutDirection::Vertical)
                                 ? contentRect.height
                                 : contentRect.width;
    float totalPreferredMainAxisSize = 0.f;

    // ! fixme: cache preferred sizes to avoid double calculation
    for (const auto& child: m_children) {
        LayoutSize childPreferredSize = child->preferredSize(
                LayoutConstraints{
                        0.f,
                        contentRect.width,
                        0.f,
                        contentRect.height,
                });
        LayoutBorders margin = child->margin();
        totalPreferredMainAxisSize +=
                (m_direction == BoxLayoutDirection::Vertical)
                        ? childPreferredSize.height + margin.top + margin.bottom
                        : childPreferredSize.width + margin.left + margin.right;
    }

    float remainingSpace = mainAxisSize - totalPreferredMainAxisSize;
    float offset = 0.f;
    float spacing = 0.f;
    if (m_justify == BoxJustify::Center) {
        offset = remainingSpace / 2.f;
    } else if (m_justify == BoxJustify::End) {
        offset = remainingSpace;
    } else if (m_justify == BoxJustify::SpaceBetween &&
               m_children.size() > 1) {
        spacing = remainingSpace / (m_children.size() - 1);
    }
    float currentPos = (m_direction == BoxLayoutDirection::Vertical)
                               ? contentRect.y + offset
                               : contentRect.x + offset;


    for (auto& child: m_children) {
        LayoutSize childPreferredSize = child->preferredSize(
                LayoutConstraints{0.f, contentRect.width, 0.f, contentRect.height});

        float childMainMargin = (m_direction == BoxLayoutDirection::Vertical)
                                        ? (child->margin().top + child->margin().bottom)
                                        : (child->margin().left + child->margin().right);
        float childCrossMargin = (m_direction == BoxLayoutDirection::Vertical)
                                         ? (child->margin().left + child->margin().right)
                                         : (child->margin().top + child->margin().bottom);

        float childW = childPreferredSize.width;
        float childH = childPreferredSize.height;

        float availableCrossSize;
        float preferredCrossSize;

        float crossAxisOffset = 0.f;
        float finalCrossSize;

        float childX = 0.f;
        float childY = 0.f;

        if (m_direction == BoxLayoutDirection::Vertical) {
            availableCrossSize = contentRect.width;
            preferredCrossSize = childW;

        } else {
            availableCrossSize = contentRect.height;
            preferredCrossSize = childH;
        }

        float availableCrossContentSpace = availableCrossSize - childCrossMargin;

        if (m_align == BoxAlign::Stretch) {
            finalCrossSize = availableCrossContentSpace;
        } else {
            finalCrossSize = preferredCrossSize;

            float remainingCrossSpace = availableCrossContentSpace - finalCrossSize;

            if (m_align == BoxAlign::Center) {
                crossAxisOffset = remainingCrossSpace / 2.0f;
            } else if (m_align == BoxAlign::End) {
                crossAxisOffset = remainingCrossSpace;
            }
        }

        if (m_direction == BoxLayoutDirection::Vertical) {
            childY = currentPos + child->margin().top;
            childX = contentRect.x + child->margin().left + crossAxisOffset;
            childW = finalCrossSize;
        } else {
            childX = currentPos + child->margin().left;
            childY = contentRect.y + child->margin().top + crossAxisOffset;
            childH = finalCrossSize;
        }

        LayoutRect childRectAssigned = {
                childX,
                childY,
                childW,
                childH};

        child->layout(childRectAssigned);

        float occupiedMainSpace = (m_direction == BoxLayoutDirection::Vertical)
                                          ? childH + childMainMargin
                                          : childW + childMainMargin;

        currentPos += occupiedMainSpace;

        if (m_justify == BoxJustify::SpaceBetween &&
            child != m_children.back()) {
            currentPos += spacing;
        }
    }

    m_calculatedBounds = rectAssigned;
}

MOE_END_NAMESPACE