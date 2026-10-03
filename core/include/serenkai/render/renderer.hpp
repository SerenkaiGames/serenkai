#pragma once
#include "serenkai/gui/label.hpp"
#include "serenkai/render/text_renderer.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <memory>
namespace serenkai {

class GuiContext;

struct RendererConfig {
    /// Excessively high frame rates can cause bugs; it's best to enable
    /// vertical sync.
    bool v_sync = true;
};

/// @brief Renderer
///
/// Initializes the renderer in the constructor and destroys it in the
/// destructor.
/// @note Must be created after the SDL video subsystem is initialized.
class Renderer {
public:
    Renderer(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    Renderer(SDL_Window* window, RendererConfig config);
    ~Renderer();

    void clear();
    void present();

    void render_lable(const Lable& lable, GuiContext& context);

private:
    const RendererConfig m_config;
    SDL_Renderer* m_sdl_renderer{nullptr};
    SDL_Color m_clear_color = {0, 0, 0, SDL_ALPHA_OPAQUE};
    std::unique_ptr<TextRenderer> m_text_renderer;
};
} // namespace serenkai
