#include "serenkai/render/text_renderer.hpp"

#include "serenkai/render/texture.hpp"
#include "serenkai/resource/font.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_surface.h>
#include <cmath>

namespace serenkai {
TextRenderer::TextRenderer(SDL_Renderer* renderer) : m_renderer(renderer) {}
TextRenderer::~TextRenderer() { clear_cache(); }

void TextRenderer::draw_text(Font& font, std::string_view utf8, int x, int y,
                             SDL_Color color) {
    auto glyphs = font.shape(utf8);
    if (glyphs.empty()) {
        return;
    }

    // HarfBuzz coordinates are in 26.6 fixed-point format; divide by 64 to get
    // pixels.
    float pen_x = static_cast<float>(x);
    float pen_y = static_cast<float>(y);

    for (const auto& g : glyphs) {
        const Font::GlyphBitmap& bmp = font.get_glyph_bitmap(g.glyph_id);

        // Whitespace and similar characters have no bitmap, so they only
        // advance the pen position.
        if (bmp.valid && bmp.width > 0 && bmp.height > 0) {
            SDL_Texture* tex = get_texture(font, g.glyph_id, bmp.width,
                                           bmp.height, bmp.pixels.data());
            if (tex) {

                // Glyph top-left = pen position + bearing + HarfBuzz offset
                // The y-axis points down in SDL and up in FreeType, so bearingY
                // must be subtracted.
                float gx = pen_x + bmp.bearing_x + g.x_offset / 64.0f;
                float gy = pen_y - bmp.bearing_y - g.y_offset / 64.0f;

                // Round to integer coordinates to avoid LINEAR sampling landing
                // between pixels and causing blurriness.
                SDL_FRect dst{std::floor(gx), std::floor(gy),
                              static_cast<float>(bmp.width),
                              static_cast<float>(bmp.height)};

                SDL_SetTextureColorMod(tex, color.r, color.g, color.b);
                SDL_SetTextureAlphaMod(tex, color.a);
                SDL_RenderTexture(m_renderer, tex, nullptr, &dst);
            }
        }

        pen_x += g.x_advance / 64.0f;
        pen_y += g.y_advance / 64.0f;
    }
}

int TextRenderer::measure_width(Font& font, std::string_view utf8) {
    int total_26_6 = 0;
    for (const auto& g : font.shape(utf8)) {
        total_26_6 += g.x_advance;
    }
    // 26.6 → pixel
    return (total_26_6 + 32) / 64;
}

void TextRenderer::clear_cache() { m_tex_cache.clear(); }

SDL_Texture* TextRenderer::get_texture(Font& font, uint32_t glyph_id, int width,
                                       int height, const uint8_t* alpha) {
    auto key = Key{glyph_id, font.pixel_size()};
    auto it = m_tex_cache.find(key);

    if (it != m_tex_cache.end()) {
        return it->second.tex.get();
    }

    if (width <= 0 || height <= 0) {
        m_tex_cache[key] = {{}, 0, 0};
        return nullptr;
    }

    std::vector<uint8_t> expanded(static_cast<size_t>(width) * height * 4);

    for (int y = 0; y < height; ++y) {
        const uint8_t* src = alpha + y * width;
        uint8_t* dst = expanded.data() + y * width * 4;
        for (int x = 0; x < width; ++x) {
            dst[x * 4 + 0] = 255;
            dst[x * 4 + 1] = 255;
            dst[x * 4 + 2] = 255;
            dst[x * 4 + 3] = src[x];
        }
    }

    SDL_PropertiesID props = SDL_CreateProperties();
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_FORMAT_NUMBER,
                          SDL_PIXELFORMAT_ARGB8888);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_ACCESS_NUMBER,
                          SDL_TEXTUREACCESS_STATIC);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_WIDTH_NUMBER, width);
    SDL_SetNumberProperty(props, SDL_PROP_TEXTURE_CREATE_HEIGHT_NUMBER, height);
    SDL_Texture* tex = SDL_CreateTextureWithProperties(m_renderer, props);
    SDL_DestroyProperties(props);

    if (!tex) {
        m_tex_cache[key] = {{}, 0, 0};
        return nullptr;
    }

    SDL_UpdateTexture(tex, nullptr, expanded.data(), width * 4);

    SDL_SetTextureBlendMode(tex, SDL_BLENDMODE_BLEND);
    SDL_SetTextureScaleMode(tex, SDL_SCALEMODE_LINEAR);

    m_tex_cache[key] = {Texture(tex), width, height};
    return tex;
}

} // namespace serenkai