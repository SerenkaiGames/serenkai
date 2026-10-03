#include "serenkai/resource/font.hpp"

#include <cstddef>
#include <fmt/format.h>
#include <glm/ext/vector_int2.hpp>
#include <stdexcept>

namespace serenkai {

Font::Font(std::string path, size_t pixel_size, FT_Library lib) {
    if (FT_New_Face(lib, path.c_str(), 0, &m_face) != 0) {
        throw std::runtime_error(fmt::format("Failed to load font {}", path));
    }

    FT_Set_Pixel_Sizes(m_face, 0, static_cast<FT_UInt>(pixel_size));
    m_pixel_size = pixel_size;
    m_hb_font = hb_ft_font_create_referenced(m_face);
    if (!m_hb_font) {
        FT_Done_Face(m_face);
        m_face = nullptr;
        throw std::runtime_error(
            fmt::format("Failed to create hb font {}", path));
    }

    // Set the HarfBuzz scale to match the FreeType pixel size.
    hb_font_set_scale(m_hb_font, pixel_size * 64, pixel_size * 64);
}
Font::~Font() {
    if (m_hb_font) {
        hb_font_destroy(m_hb_font);
        m_hb_font = nullptr;
    }
    if (m_face) {
        FT_Done_Face(m_face);
        m_face = nullptr;
    }
    m_bitmap_cache.clear();
    m_pixel_size = 0;
}

std::vector<Font::ShapedGlyph> Font::shape(std::string_view utf8) const {
    std::vector<ShapedGlyph> result;
    if (!m_hb_font || utf8.empty())
        return result;

    hb_buffer_t* buf = hb_buffer_create();
    hb_buffer_add_utf8(buf, utf8.data(), static_cast<int>(utf8.size()), 0, -1);
    hb_buffer_guess_segment_properties(buf);
    hb_shape(m_hb_font, buf, nullptr, 0);

    unsigned int count = 0;
    const hb_glyph_info_t* info = hb_buffer_get_glyph_infos(buf, &count);
    const hb_glyph_position_t* pos = hb_buffer_get_glyph_positions(buf, &count);

    result.reserve(count);
    for (unsigned int i = 0; i < count; ++i) {
        ShapedGlyph g;
        g.glyph_id = info[i].codepoint;
        g.x_advance = pos[i].x_advance;
        g.y_advance = pos[i].y_advance;
        g.x_offset = pos[i].x_offset;
        g.y_offset = pos[i].y_offset;
        result.push_back(g);
    }

    hb_buffer_destroy(buf);
    return result;
}

const Font::GlyphBitmap& Font::get_glyph_bitmap(uint32_t glyph_id) {
    auto it = m_bitmap_cache.find(glyph_id);
    if (it != m_bitmap_cache.end()) {
        return it->second;
    }

    GlyphBitmap gb;

    if (!m_face || FT_Load_Glyph(m_face, glyph_id, FT_LOAD_RENDER) != 0) {
        return m_bitmap_cache.emplace(glyph_id, std::move(gb)).first->second;
    }

    const FT_Bitmap& bmp = m_face->glyph->bitmap;
    gb.width = static_cast<int>(bmp.width);
    gb.height = static_cast<int>(bmp.rows);
    gb.bearing_x = m_face->glyph->bitmap_left;
    gb.bearing_y = m_face->glyph->bitmap_top;
    gb.valid = true;

    if (gb.width > 0 && gb.height > 0) {
        gb.pixels.resize(static_cast<size_t>(gb.width) * gb.height);
        for (int y = 0; y < gb.height; ++y) {
            const uint8_t* src = bmp.buffer + y * bmp.pitch;
            uint8_t* dst = gb.pixels.data() + y * gb.width;
            std::copy(src, src + gb.width, dst);
        }
    }

    auto [ins, _] = m_bitmap_cache.emplace(glyph_id, std::move(gb));
    return ins->second;
}

int Font::measure_width(std::string_view utf8) {
    int total_26_6 = 0;
    for (const auto& g : shape(utf8)) {
        total_26_6 += g.x_advance;
    }
    // 26.6 → pixel
    return (total_26_6 + 32) / 64;
}

size_t Font::pixel_size() const { return m_pixel_size; }

int Font::ascender() const {
    if (!m_face)
        return 0;
    return static_cast<int>(m_face->size->metrics.ascender >> 6);
}

int Font::descender() const {
    if (!m_face)
        return 0;
    return static_cast<int>(m_face->size->metrics.descender >> 6);
}

int Font::line_height() const {
    if (!m_face)
        return 0;
    return static_cast<int>(m_face->size->metrics.height >> 6);
}

FT_Face Font::ft_face() const { return m_face; }
hb_font_t* Font::hb_font() const { return m_hb_font; }

} // namespace serenkai