#pragma once
#include "serenkai/gui/color.hpp"
#include "serenkai/gui/label.hpp"
#include "serenkai/render/text_renderer.hpp"

#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <glm/ext/vector_int2.hpp>
#include <memory>
namespace serenkai {

class GuiContext;
class TextureManager;

struct RendererConfig {
    /// Excessively high frame rates can cause bugs; it's best to enable
    /// vertical sync.
    bool v_sync = true;

    SDL_Window* window = nullptr;
    TextureManager* texture_manager = nullptr;
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

    Renderer(const RendererConfig& config);
    ~Renderer();

    SDL_Renderer* get_sdl_renderer() const;

    void clear();
    void present();

    /// @param pos  pos is in physical coordinates
    /// @param size size is the logical size
    /// @note Please pass physical coordinates, not logical coordinates.
    void draw_text(Font& font, std::string_view utf8, glm::ivec2 pos,
                   Color color, float scale);

    /// @param pos  pos is in physical coordinates
    /// @param size size is the logical size
    /// @note Please pass physical coordinates, not logical coordinates.
    void draw_rect(glm::ivec2 pos, glm::ivec2 size, Color color, float alpha,
                   float scale);

private:
    const RendererConfig m_config;
    SDL_Renderer* m_sdl_renderer{nullptr};
    TextureManager* m_texture_manager{nullptr};
    SDL_Color m_clear_color = {0, 0, 0, SDL_ALPHA_OPAQUE};
    std::unique_ptr<TextRenderer> m_text_renderer;
};
} // namespace serenkai
