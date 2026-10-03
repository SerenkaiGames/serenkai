#pragma once

namespace serenkai {
class Renderer;

/// @brief The Gui Context is used to provide GUI-related classes
///
/// It is passed to relevant classes through dependency injection.
class GuiContext {
public:
    GuiContext(Renderer* renderer);
    Renderer* get_renderer() const;

private:
    Renderer* m_renderer = nullptr;
};
} // namespace serenkai