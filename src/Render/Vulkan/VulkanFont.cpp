#include "Render/Vulkan/VulkanFont.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"

#include <ft2build.h>
#include FT_FREETYPE_H

namespace moe {
    // Ascii and Latin
    constexpr uint16_t DEFAULT_GLYPH_CODEPOINT_RANGE = 255;

    std::u32string generateDefaultGlyphRange() {
        static std::u32string cached{};
        if (!cached.empty()) {
            return cached;
        }

        std::basic_stringstream<char32_t> ss;
        // ascii
        for (int i = 0; i < DEFAULT_GLYPH_CODEPOINT_RANGE; i++) {
            ss << static_cast<char32_t>(i);
        }
        cached = ss.str();

        return cached;
    }

    void VulkanFont::init(VulkanEngine& engine, Span<uint8_t> fontData, float fontSize, StringView glyphRanges) {
        MOE_ASSERT(!fontData.empty(), "Font data is empty");
        MOE_ASSERT(fontSize > 0.0f, "Font size must be greater than 0");
        MOE_ASSERT(!m_initialized, "Font is already initialized");

        std::u32string glyphRanges32;
        if (glyphRanges.empty()) {
            auto defaultGlyphRange = generateDefaultGlyphRange();
            glyphRanges32 = defaultGlyphRange;
        } else {
            glyphRanges32 = utf8::utf8to32(glyphRanges);
        }

        m_engine = &engine;

        FT_Library ft;
        if (FT_Init_FreeType(&ft)) {
            Logger::error("Could not init FreeType Library");
            return;
        }

        FT_Face face;
        if (FT_New_Memory_Face(ft, fontData.data(), fontData.size(), 0, &face)) {
            Logger::error("Failed to load font from memory");
            FT_Done_FreeType(ft);
            return;
        }

        if (FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(fontSize))) {
            Logger::error("Failed to set font pixel size");
            FT_Done_Face(face);
            FT_Done_FreeType(ft);
            return;
        }

        FontImageBuffer fontImageBuffer(static_cast<int>(glyphRanges32.size()), fontSize);
        Logger::debug("FontImageBuffer: {}x{} pixels, can hold {} glyphs",
                      fontImageBuffer.widthInPixels,
                      fontImageBuffer.heightInPixels,
                      fontImageBuffer.maxGlyphCount);

        for (char32_t c: glyphRanges32) {
            if (c < DEFAULT_GLYPH_CODEPOINT_RANGE && !std::isprint(static_cast<char>(c))) {
                continue;
            }

            auto charGlyphIdx = FT_Get_Char_Index(face, c);
            if (FT_Load_Glyph(face, charGlyphIdx, FT_LOAD_RENDER)) {
                Logger::warn("Failed to load Glyph {}", static_cast<uint32_t>(c));
                continue;
            }

            if (FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL)) {
                Logger::warn("Failed to render Glyph {}", static_cast<uint32_t>(c));
                continue;
            }

            auto& charInfo = m_characters[c];
            fontImageBuffer.addGlyph(
                    face->glyph->bitmap.buffer,
                    face->glyph->bitmap.width,
                    face->glyph->bitmap.rows,
                    &charInfo.uvOffset.x,
                    &charInfo.uvOffset.y);

            Logger::debug(
                    "Loaded glyph '{}' : uv offset ({}, {}), px offset ({}, {})",
                    static_cast<uint32_t>(c),
                    charInfo.uvOffset.x, charInfo.uvOffset.y,
                    charInfo.uvOffset.x * fontImageBuffer.widthInPixels, charInfo.uvOffset.y * fontImageBuffer.heightInPixels);

            charInfo.size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
            charInfo.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
            charInfo.advance = static_cast<uint32_t>(face->glyph->advance.x);

            m_characters.emplace(c, charInfo);
        }

        // upload to gpu
        ImageId fontImageId = engine.m_caches.imageCache.loadImageFromMemory(
                Span<uint8_t>(fontImageBuffer.pixels, fontImageBuffer.widthInPixels * fontImageBuffer.heightInPixels),
                {static_cast<uint32_t>(fontImageBuffer.widthInPixels), static_cast<uint32_t>(fontImageBuffer.heightInPixels)},
                VK_FORMAT_R8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT);
        m_fontImageId = fontImageId;

        m_initialized = true;

        FT_Done_Face(face);
        FT_Done_FreeType(ft);
    }

    void VulkanFont::destroy() {
    }
}// namespace moe