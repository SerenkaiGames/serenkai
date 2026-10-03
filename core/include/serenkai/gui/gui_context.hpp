#pragma once

#include "serenkai/application/event.hpp"
#include "serenkai/resource/font.hpp"

#include <cstddef>
#include <glm/ext/vector_int2.hpp>
#include <memory>
#include <string_view>
namespace serenkai {
class Renderer;

struct GuiConfig {
    Renderer* renderer;
    FT_Library lib;
    std::string_view font_path;
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

private:
    std::unique_ptr<Font> m_font;
    Renderer* m_renderer = nullptr;

    size_t m_ui_scale = 3;
    glm::ivec2 m_logical_window_size{0};
};
} // namespace serenkai