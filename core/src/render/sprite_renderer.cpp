#include "serenkai/render/sprite_renderer.hpp"

#include "serenkai/render/render_utils.hpp"

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>

namespace serenkai {
SpriteRenderer::SpriteRenderer(SDL_Renderer* renderer) : m_renderer(renderer) {}

void SpriteRenderer::render_sprite(SDL_Texture* texture, glm::vec2 world_pos,
                                   glm::vec2 size, glm::vec2 screen_offset,
                                   float zoom,
                                   std::optional<glm::vec4> src_rect,
                                   FlipFlag flip) {

    if (!texture || !m_renderer) {
        return;
    }

    SDL_FRect dst{world_pos.x * zoom + screen_offset.x,
                  world_pos.y * zoom + screen_offset.y, size.x * zoom,
                  size.y * zoom};

    const SDL_FRect* src_ptr = nullptr;
    SDL_FRect src{};
    if (src_rect) {
        src = {src_rect->x, src_rect->y, src_rect->z, src_rect->w};
        src_ptr = &src;
    }

    auto [angle, sdl_flip] = resolve_flip_transform(flip, dst);

    SDL_RenderTextureRotated(m_renderer, texture, src_ptr, &dst, angle, nullptr,
                             sdl_flip);
}

} // namespace serenkai