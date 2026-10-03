#pragma once

#include "serenkai/application/event.hpp"

#include <cstddef>
#include <glm/ext/vector_int2.hpp>
namespace serenkai {
class Renderer;

/// @brief The Gui Context is used to provide GUI-related classes
///
/// It is passed to relevant classes through dependency injection.
class GuiContext {
public:
    GuiContext(Renderer* renderer);
    Renderer* get_renderer() const;

    /// @brief Update the UI scaling factor and logical resolution size.
    bool handle_window_resize_event(const WindowResizeEvent& e);

private:
    Renderer* m_renderer = nullptr;
    size_t m_ui_scale = 3;
    glm::ivec2 m_logical_window_size{0};
};
} // namespace serenkai