#include "serenkai/render/renderer.hpp"

#include <SDL3/SDL_error.h>
#include <SDL3/SDL_render.h>
#include <fmt/format.h>
#include <stdexcept>

namespace serenkai {
Renderer::Renderer(SDL_Window* window, RendererConfig config)
    : m_config(std::move(config)) {
    m_sdl_renderer = SDL_CreateRenderer(window, nullptr);
    if (!m_sdl_renderer) {
        throw std::runtime_error(fmt::format(
            "Failed to initialize SDL_Renderer, error: {}", SDL_GetError()));
    }

    SDL_SetRenderVSync(m_sdl_renderer, static_cast<int>(config.v_sync));
}
Renderer::~Renderer() {
    if (m_sdl_renderer) {
        SDL_DestroyRenderer(m_sdl_renderer);
    }
}

void Renderer::present() { SDL_RenderPresent(m_sdl_renderer); }

void Renderer::clear() {
    SDL_SetRenderDrawColor(m_sdl_renderer, m_clear_color.r, m_clear_color.g,
                           m_clear_color.b, m_clear_color.a);

    SDL_RenderClear(m_sdl_renderer);
}
} // namespace serenkai
