#pragma once

#include "Render/Vulkan/VulkanTypes.hpp"

#include <utf8.h>

namespace moe {
    struct VulkanEngine;
};

namespace moe {
    struct VulkanFont {
    public:
        VulkanFont() = default;

        void init(VulkanEngine& engine, Span<uint8_t> fontData, float fontSize, StringView glyphRanges);

        void destroy();

    private:
        struct Character {
            glm::vec2 uvOffset;
            glm::ivec2 size;
            glm::ivec2 bearing;
            uint32_t advance;
        };

        struct FontImageBuffer {
            uint8_t* pixels{nullptr};

            int widthInPixels{0};
            int heightInPixels{0};

            int glyphPerRow{0};
            int glyphPerColumn{0};

            int glyphCellSize{0};

            int maxGlyphCount{0};
            int currentGlyphCount{0};

            FontImageBuffer(int glyphCount, float fontSize) {
                glyphCellSize = static_cast<int>(fontSize) + 2;// +2 for padding
                glyphPerRow = static_cast<int>(glm::ceil(glm::sqrt(static_cast<float>(glyphCount))));
                glyphPerColumn = static_cast<int>(glm::ceil(static_cast<float>(glyphCount) / static_cast<float>(glyphPerRow)));

                widthInPixels = glyphPerRow * glyphCellSize;
                heightInPixels = glyphPerColumn * glyphCellSize;

                maxGlyphCount = glyphPerRow * glyphPerColumn;
                currentGlyphCount = 0;

                pixels = new uint8_t[widthInPixels * heightInPixels];
                std::memset(pixels, 0, widthInPixels * heightInPixels);
            }

            void addGlyph(uint8_t* glyphPixels, int glyphWidth, int glyphHeight, float* outUVOffsetX, float* outUVOffsetY) {
                if (currentGlyphCount >= maxGlyphCount) {
                    Logger::error("FontImageBuffer: Exceeded maximum glyph count");
                    return;
                }

                int row = currentGlyphCount / glyphPerRow;
                int col = currentGlyphCount % glyphPerRow;

                *outUVOffsetX = row / static_cast<float>(glyphPerRow);
                *outUVOffsetY = col / static_cast<float>(glyphPerColumn);

                for (int y = 0; y < glyphHeight; ++y) {
                    for (int x = 0; x < glyphWidth; ++x) {
                        int destX = col * glyphCellSize + x + 1;// +1 for padding
                        int destY = row * glyphCellSize + y + 1;
                        pixels[destY * widthInPixels + destX] = glyphPixels[y * glyphWidth + x];
                    }
                }

                currentGlyphCount++;
            }

            ~FontImageBuffer() {
                if (pixels) {
                    delete[] pixels;
                    pixels = nullptr;
                }
            }
        };

        StringView m_path;
        VulkanEngine* m_engine{nullptr};
        bool m_initialized{false};

        UnorderedMap<char32_t, Character> m_characters;
        ImageId m_fontImageId{NULL_IMAGE_ID};
    };
}// namespace moe