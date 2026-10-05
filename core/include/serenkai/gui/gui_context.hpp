#pragma once

#include "serenkai/application/event.hpp"
#include "serenkai/gui/image_widget.hpp"
#include "serenkai/gui/label.hpp"
#include "serenkai/gui/rect.hpp"

#include <cstddef>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_int2.hpp>
namespace serenkai {
class Renderer;
class TextureManager;
struct GuiConfig {
    Renderer* renderer{nullptr};
    TextureManager* texture_manager{nullptr};
};

/// @brief The Gui Context is used to provide GUI-related classes
///
/// It is passed to relevant classes through dependency injection.
class GuiContext {
public:
    GuiContext(GuiConfig config);
    Renderer* get_renderer() const;

    size_t ui_scale() const;

    /// @brief Update the UI scaling factor and logical resolution size.
    bool handle_window_resize_event(const WindowResizeEvent& e);

    [[nodiscard]] glm::ivec2 to_physical_coord(glm::ivec2 pos) const;
    [[nodiscard]] glm::vec2 to_logical_coord(glm::vec2 physical_pos) const;

    void render_label(const Label& label);
    void render_rect(const Rect& rect);
    void render_image(const ImageWidget& image);

private:
    Renderer* m_renderer = nullptr;
    TextureManager* m_texture_manager = nullptr;
    size_t m_ui_scale = 3;
    glm::ivec2 m_logical_window_size{0};
};
} // namespace serenkai