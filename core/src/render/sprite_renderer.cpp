#include "serenkai/render/sprite_renderer.hpp"

#include <SDL3/SDL_rect.h>
#include <SDL3/SDL_render.h>

namespace serenkai {
SpriteRenderer::SpriteRenderer(SDL_Renderer* renderer) : m_renderer(renderer) {}

void SpriteRenderer::render_sprite(SDL_Texture* texture, glm::vec2 world_pos,
                                   glm::vec2 size, glm::vec2 screen_offset,
                                   float zoom) {

    if (!texture || !m_renderer) {
        return;
    }

    SDL_FRect dst{world_pos.x * zoom + screen_offset.x,
                  world_pos.y * zoom + screen_offset.y, size.x * zoom,
                  size.y * zoom};

    SDL_RenderTexture(m_renderer, texture, nullptr, &dst);
}

} // namespace serenkai