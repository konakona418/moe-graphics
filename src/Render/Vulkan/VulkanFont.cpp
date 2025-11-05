#include "Render/Vulkan/VulkanFont.hpp"
#include "Render/Vulkan/VulkanEngine.hpp"

#include "Render/Vulkan/VulkanUtils.hpp"

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

        Vector<char32_t> ss;
        // ascii
        for (int i = 0; i < DEFAULT_GLYPH_CODEPOINT_RANGE; i++) {
            ss.push_back(static_cast<char32_t>(i));
        }
        cached = std::u32string(ss.begin(), ss.end());

        return cached;
    }

    bool VulkanFont::loadFontInternal(std::u32string_view glyphRanges32) {
        FT_Library ft;
        if (FT_Init_FreeType(&ft)) {
            Logger::error("Could not init FreeType Library");
            return false;
        }

        FT_Face face;
        if (FT_New_Memory_Face(ft, m_fontData, m_fontDataSize, 0, &face)) {
            Logger::error("Failed to load font from memory");
            FT_Done_FreeType(ft);
            return false;
        }

        if (FT_Set_Pixel_Sizes(face, 0, static_cast<FT_UInt>(m_fontSize))) {
            Logger::error("Failed to set font pixel size");
            FT_Done_Face(face);
            FT_Done_FreeType(ft);
            return false;
        }

        for (char32_t c: glyphRanges32) {
            if (c < DEFAULT_GLYPH_CODEPOINT_RANGE && !std::isprint(static_cast<char>(c))) {
                // not printable
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
            bool result = m_fontImageBufferCPU->addGlyph(
                    face->glyph->bitmap.buffer,
                    face->glyph->bitmap.width,
                    face->glyph->bitmap.rows,
                    &charInfo.uvOffset.x,
                    &charInfo.uvOffset.y);
            if (!result) {
                Logger::warn("Failed to add Glyph {} to font image buffer", static_cast<uint32_t>(c));
                continue;
            }

            charInfo.pxOffset = {
                    charInfo.uvOffset.x * m_fontImageBufferCPU->widthInPixels + CELL_PADDING,
                    charInfo.uvOffset.y * m_fontImageBufferCPU->heightInPixels + CELL_PADDING,
            };

            charInfo.size = glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows);
            charInfo.bearing = glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top);
            charInfo.advance = static_cast<uint32_t>(face->glyph->advance.x);

            m_characters.emplace(c, charInfo);
        }

        FT_Done_Face(face);
        FT_Done_FreeType(ft);

        return true;
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
        m_fontSize = fontSize;

        m_fontData = new uint8_t[fontData.size()];
        m_fontDataSize = fontData.size();
        std::memcpy(m_fontData, fontData.data(), fontData.size());

        m_fontImageBufferCPU = std::make_unique<FontImageBuffer>(static_cast<int>(glyphRanges32.size()), m_fontSize);
        m_fontImageBuffer.init(*m_engine,
                               VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                               m_fontImageBufferCPU->widthInPixels * m_fontImageBufferCPU->heightInPixels,
                               1);

        auto basicGlyphRange = generateDefaultGlyphRange();
        loadFontInternal(basicGlyphRange);

        VulkanAllocatedImage image = engine.allocateImage(
                {
                        static_cast<uint32_t>(m_fontImageBufferCPU->widthInPixels),
                        static_cast<uint32_t>(m_fontImageBufferCPU->heightInPixels),
                        1,
                },
                VK_FORMAT_R8_UNORM,
                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
                false);

        // upload to gpu
        ImageId fontImageId = engine.m_caches.imageCache.addImage(std::move(image));
        m_fontImageId = fontImageId;

        m_initialized = true;

        m_engine->immediateSubmit([&](VkCommandBuffer cmd) {
            m_fontImageBuffer.upload(cmd, m_fontImageBufferCPU->pixels, 0, m_fontImageBufferCPU->sizeInBytes());
            VkUtils::copyFromBufferToImage(
                    cmd,
                    m_fontImageBuffer.getBuffer().buffer,
                    engine.m_caches.imageCache.getImage(m_fontImageId)->image,
                    VkExtent3D{
                            static_cast<uint32_t>(m_fontImageBufferCPU->widthInPixels),
                            static_cast<uint32_t>(m_fontImageBufferCPU->heightInPixels),
                            1},
                    false);
        });

        Logger::debug(
                "Initialized font with {} glyphs, atlas size: {}x{}",
                m_characters.size(),
                m_fontImageBufferCPU->widthInPixels, m_fontImageBufferCPU->heightInPixels);
    }

    bool VulkanFont::lazyLoadCharacters() {
        if (m_pendingLazyLoadGlyphs.empty()) {
            return true;
        }

        Logger::debug("Lazy loading {} glyphs for font", m_pendingLazyLoadGlyphs.size());
        std::u32string glyphRanges32(m_pendingLazyLoadGlyphs.begin(), m_pendingLazyLoadGlyphs.end());
        m_pendingLazyLoadGlyphs.clear();

        size_t originalGlyphCount = m_fontImageBufferCPU->currentGlyphCount;
        loadFontInternal(glyphRanges32);

        size_t newGlyphCount = m_fontImageBufferCPU->currentGlyphCount - originalGlyphCount;
        auto copyRange = m_fontImageBufferCPU->getCopyRange(originalGlyphCount);

        m_engine->immediateSubmit([=](VkCommandBuffer cmd) {
            m_fontImageBuffer.upload(
                    cmd,
                    m_fontImageBufferCPU->pixels + copyRange.offset,
                    0,
                    copyRange.size,
                    copyRange.offset);

            // here we copy:
            // |----------existing glyphs------------|
            // :-> copyRange.offset
            // |----existing glyphs---|--new glyphs--|
            // |----new glyphs---|--not initialized--|
            // :-> copyRange.offset + copyRange.size

            VkUtils::copyFromBufferToImage(
                    cmd,
                    m_fontImageBuffer.getBuffer().buffer,
                    m_engine->m_caches.imageCache.getImage(m_fontImageId)->image,
                    VkExtent3D{
                            static_cast<uint32_t>(m_fontImageBufferCPU->widthInPixels),
                            static_cast<uint32_t>(copyRange.size / m_fontImageBufferCPU->widthInPixels),
                            1},
                    VkOffset3D{
                            0,
                            static_cast<int32_t>(copyRange.offset / m_fontImageBufferCPU->widthInPixels),
                            0},
                    copyRange.offset,
                    0,
                    0);
        });

        return true;
    }

    void VulkanFont::destroy() {
        m_fontImageBuffer.destroy();
    }
}// namespace moe