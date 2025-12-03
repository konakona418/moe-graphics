#include "UI/TextWidget.hpp"

MOE_BEGIN_NAMESPACE

LayoutSize TextWidget::preferredSize(const LayoutConstraints& constraints) const {
    // ! fixme: this assumes monospace font for simplicity
    float fontSize = m_fontSize;
    float textSize = static_cast<float>(m_text.size());

    float calculatedWidth = Math::min(textSize * fontSize, constraints.maxWidth);
    size_t maxCharsPerRow = static_cast<size_t>(calculatedWidth / fontSize);

    size_t requiredRows =
            maxCharsPerRow == 0
                    ? 1
                    : (textSize + maxCharsPerRow - 1) / maxCharsPerRow;
    float calculatedHeight = requiredRows * fontSize;
    calculatedHeight = Math::min(calculatedHeight, constraints.maxHeight);

    return clampConstraintMinMax(
            LayoutSize{calculatedWidth, calculatedHeight},
            constraints);
}

void TextWidget::layout(const LayoutRect& rectAssigned) {
    m_calculatedBounds = rectAssigned;
    m_lines.clear();

    float fontSize = m_fontSize;
    size_t textSize = m_text.size();

    size_t maxCharsPerRow = static_cast<size_t>(rectAssigned.width / fontSize);
    if (maxCharsPerRow == 0) {
        return;
    }

    size_t currentIdx = 0;
    size_t currentY = 0;
    size_t maxRows = static_cast<size_t>(rectAssigned.height / fontSize);
    while (currentIdx < textSize) {
        if (currentY + fontSize > rectAssigned.height) {
            break;
        }

        size_t endIdx = Math::min(currentIdx + maxCharsPerRow, textSize);
        U32String lineText = m_text.substr(currentIdx, endIdx - currentIdx);

        m_lines.push_back(Line{lineText, static_cast<float>(currentY)});

        currentIdx = endIdx;
        currentY += static_cast<size_t>(fontSize);
    }
}

MOE_END_NAMESPACE