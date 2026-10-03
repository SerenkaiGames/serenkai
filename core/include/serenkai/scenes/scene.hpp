#pragma once

#include "serenkai/application/event.hpp"

#include <variant>
namespace serenkai {

class GuiContext;

enum class SceneType { Title, Game };

/// @brief Base class for Scene
///
/// Provides Scene-related interfaces.
class Scene {
public:
    Scene() = default;

    Scene(const Scene&) = delete;
    Scene(Scene&&) = delete;
    Scene& operator=(const Scene&) = delete;
    Scene& operator=(Scene&&) = delete;

    virtual ~Scene() = default;

    /// @brief Update function, called once per frame
    virtual void update(float dt) = 0;

    /// @brief Render function, called after update
    virtual void render(GuiContext* context) = 0;

    /// @brief Receive event and pass down
    virtual bool handle_event(const Event& e) {
        return std::visit(Overloaded{[this](const MouseMoveEvent& e) {
                                         return handle_mouse_move_event(e);
                                     },
                                     [this](const WindowResizeEvent& e) {
                                         return handle_window_resize_event(e);
                                     },
                                     [this](const MouseWheelEvent& e) {
                                         return handle_mouse_wheel_event(e);
                                     },
                                     [this](const KeyEvent& e) {
                                         return handle_key_event(e);
                                     },
                                     [this](const TextInputEvent& e) {
                                         return handle_text_input_event(e);
                                     },
                                     [](const auto&) { return false; }},
                          e);
    }

    /// @brief Called once when entering the scene.
    virtual void on_enter() {}

    /// @brief Called once when leaving the scene, used to clean up resources.
    virtual void on_leave() {}

protected:
    virtual bool handle_mouse_move_event(const MouseMoveEvent&) {
        return false;
    }

    virtual bool handle_window_resize_event(const WindowResizeEvent&) {
        return false;
    }

    virtual bool handle_mouse_wheel_event(const MouseWheelEvent&) {
        return false;
    }

    virtual bool handle_key_event(const KeyEvent&) { return false; }

    virtual bool handle_text_input_event(const TextInputEvent&) {
        return false;
    }
};
} // namespace serenkai
