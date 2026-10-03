#pragma once

#include <cstddef>
#include <cstdint>
#include <ft2build.h>
#include <string>
#include <unordered_map>
#include FT_FREETYPE_H
#include <hb-ft.h>
#include <hb.h>
#include <vector>

namespace serenkai {

/// @brief Font loading manager class
///
/// Creates the font on construction
/// Releases it on destruction
class Font {
public:
    // Rasterized glyph bitmap (8-bit alpha, width * height)
    struct GlyphBitmap {
        std::vector<uint8_t> pixels;
        int width = 0;
        int height = 0;
        int bearing_x = 0; // Horizontal offset of the left edge relative to the
                           // pen position
        int bearing_y = 0; // Vertical offset of the top edge relative to the
                           // baseline (positive upward)
        bool valid = false; // For an empty glyph (e.g., a space), the bitmap is
                            // empty but valid = true
    };

    // HarfBuzz shaping result; coordinates are 26.6 fixed-point numbers (divide
    // by 64 to get pixels)
    struct ShapedGlyph {
        uint32_t glyph_id = 0;
        int x_advance = 0;
        int y_advance = 0;
        int x_offset = 0;
        int y_offset = 0;
    };

    Font(const Font&) = delete;
    Font(Font&&) = delete;
    Font& operator=(const Font&) = delete;
    Font& operator=(Font&&) = delete;

    Font(std::string path, size_t pixel_size, FT_Library lib);
    ~Font();

    /// @brief Shape a UTF-8 text and return a glyph sequence
    std::vector<ShapedGlyph> shape(std::string_view utf8) const;

    /// @brief Get the glyph bitmap, cached internally
    const GlyphBitmap& get_glyph_bitmap(uint32_t glyph_id);

    // Metrics
    size_t pixel_size() const;
    int ascender() const;
    int descender() const; // negative value
    int line_height() const;

    // Underlying handles
    FT_Face ft_face() const;
    hb_font_t* hb_font() const;

private:
    FT_Face m_face = nullptr;
    hb_font_t* m_hb_font = nullptr;
    size_t m_pixel_size = 0;

    // Rasterized glyph bitmap cache, key = glyph id
    std::unordered_map<uint32_t, GlyphBitmap> m_bitmap_cache;
};
} // namespace serenkai