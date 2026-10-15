#pragma once

#include <SDL3/SDL_render.h>
#include <glm/ext/vector_float2.hpp>
namespace serenkai {
class SpriteRenderer {
public:
    SpriteRenderer(SDL_Renderer* renderer);

    void render_sprite(SDL_Texture* texture, glm::vec2 world_pos,
                       glm::vec2 size, glm::vec2 screen_offset, float zoom);

private:
    SDL_Renderer* m_renderer = nullptr;
};
} // namespace serenkai