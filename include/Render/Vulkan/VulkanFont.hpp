#pragma once

#include "Core/ResourceCache.hpp"
#include "Render/Vulkan/VulkanIdTypes.hpp"
#include "Render/Vulkan/VulkanTypes.hpp"

#include <utf8.h>

namespace moe {
    struct VulkanEngine;
};

namespace moe {
    struct VulkanFont {
    public:
        struct Character {
            glm::vec2 uvOffset;
            glm::vec2 pxOffset;
            glm::ivec2 size;
            glm::ivec2 bearing;
            uint32_t advance;
        };

        VulkanFont() = default;

        void init(VulkanEngine& engine, Span<uint8_t> fontData, float fontSize, StringView glyphRanges);

        void destroy();

        UnorderedMap<char32_t, Character>& getCharacters() { return m_characters; }
        ImageId getFontImageId() const { return m_fontImageId; }
        float getFontSize() const { return m_fontSize; }

    private:
        static constexpr int32_t CELL_PADDING = 1;

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
                glyphCellSize = static_cast<int>(fontSize) + CELL_PADDING * 2;// +2 for padding
                glyphPerRow = static_cast<int>(glm::ceil(glm::sqrt(static_cast<float>(glyphCount))));
                glyphPerColumn = static_cast<int>(glm::ceil(static_cast<float>(glyphCount) / static_cast<float>(glyphPerRow)));

                widthInPixels = glyphPerRow * glyphCellSize;
                heightInPixels = glyphPerColumn * glyphCellSize;

                maxGlyphCount = glyphPerRow * glyphPerColumn;
                currentGlyphCount = 0;

                pixels = new uint8_t[widthInPixels * heightInPixels];
                std::memset(pixels, 0, widthInPixels * heightInPixels);
            }

            bool addGlyph(uint8_t* glyphPixels, int glyphWidth, int glyphHeight, float* outUVOffsetX, float* outUVOffsetY) {
                if (currentGlyphCount >= maxGlyphCount) {
                    Logger::error("FontImageBuffer: Exceeded maximum glyph count");
                    return false;
                }

                if (glyphWidth > glyphCellSize - CELL_PADDING * 2 || glyphHeight > glyphCellSize - CELL_PADDING * 2) {
                    Logger::warn("Glyph size exceeds cell size: glyph ({}, {}), cell ({}, {})",
                                 glyphWidth, glyphHeight,
                                 glyphCellSize, glyphCellSize);
                    return false;
                }

                int row = currentGlyphCount / glyphPerRow;
                int col = currentGlyphCount % glyphPerRow;

                *outUVOffsetX = static_cast<float>(col * glyphCellSize) / static_cast<float>(widthInPixels);
                *outUVOffsetY = static_cast<float>(row * glyphCellSize) / static_cast<float>(heightInPixels);

                for (int y = 0; y < glyphHeight; ++y) {
                    for (int x = 0; x < glyphWidth; ++x) {
                        int destX = col * glyphCellSize + x + CELL_PADDING;// +1 for padding
                        int destY = row * glyphCellSize + y + CELL_PADDING;

                        if (destX < 0 || destX >= widthInPixels || destY < 0 || destY >= heightInPixels) {
                            Logger::warn("Glyph destination out of bounds: destX={}, destY={}, width={}, height={}",
                                         destX, destY, widthInPixels, heightInPixels);
                            return false;
                        }

                        pixels[destY * widthInPixels + destX] = glyphPixels[y * glyphWidth + x];
                    }
                }

                currentGlyphCount++;

                return true;
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

        float m_fontSize{0.0f};

        UnorderedMap<char32_t, Character> m_characters;
        ImageId m_fontImageId{NULL_IMAGE_ID};
    };

    struct VulkanFontLoader {
        SharedResource<VulkanFont> operator()(VulkanEngine& engine, StringView path, Span<uint8_t> fontData, float fontSize, StringView glyphRanges) {
            auto font = std::make_shared<VulkanFont>();
            font->init(engine, fontData, fontSize, glyphRanges);
            return SharedResource<VulkanFont>(font);
        }

        SharedResource<VulkanFont> operator()(VulkanFont&& font) {
            return std::make_shared<VulkanFont>(std::forward<VulkanFont>(font));
        }
    };

    struct VulkanFontCache : public ResourceCache<FontId, VulkanFont, VulkanFontLoader> {};
}// namespace moe