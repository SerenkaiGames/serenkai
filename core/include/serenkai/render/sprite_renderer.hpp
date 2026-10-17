#pragma once

#include "serenkai/render/flip_flag.hpp"

#include <SDL3/SDL_render.h>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>
#include <optional>
namespace serenkai {
class SpriteRenderer {
public:
    SpriteRenderer(SDL_Renderer* renderer);

    void render_sprite(SDL_Texture* texture, glm::vec2 world_pos,
                       glm::vec2 size, glm::vec2 screen_offset, float zoom,
                       std::optional<glm::vec4> src_rect = std::nullopt,
                       FlipFlag flip = FlipFlag::None);

private:
    SDL_Renderer* m_renderer = nullptr;
};
} // namespace serenkai