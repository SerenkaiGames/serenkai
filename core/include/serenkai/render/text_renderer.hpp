#pragma once

#include "serenkai/base/math.hpp"
#include "serenkai/render/texture.hpp"

#include <SDL3/SDL_render.h>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>
namespace serenkai {
class Font;

/// @brief Low-level text rendering class
///
/// Used for text rendering
/// Automatically caches font textures
class TextRenderer {
public:
    TextRenderer(const TextRenderer&) = delete;
    TextRenderer(TextRenderer&&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;
    TextRenderer& operator=(TextRenderer&&) = delete;

    explicit TextRenderer(SDL_Renderer* renderer);
    ~TextRenderer();

    void draw_text(Font& font, std::string_view utf8, int x, int y,
                   SDL_FColor color, float scale);

    void clear_cache();

private:
    struct CachedTex {
        Texture tex;
        int width = 0;
        int height = 0;
    };

    struct Key {
        std::uint32_t glyph_id;
        std::size_t pixel_size;
        Key(uint32_t g, size_t p) : glyph_id(g), pixel_size(p) {}
        bool operator==(const Key&) const = default;
        struct Hash {
            std::size_t operator()(const Key& k) const noexcept {
                std::uint32_t h = 0;
                h = combine32(h, hash_to_32(std::hash<uint32_t>{}(k.glyph_id)));
                h = combine32(h, hash_to_32(std::hash<size_t>{}(k.pixel_size)));
                return fmix32(h);
            }
        };
    };

    SDL_Renderer* m_renderer = nullptr;
    std::unordered_map<Key, CachedTex, Key::Hash> m_tex_cache;

    SDL_Texture* get_texture(Font& font, uint32_t glyph_id, int width,
                             int height, const uint8_t* alpha);
};
} // namespace serenkai