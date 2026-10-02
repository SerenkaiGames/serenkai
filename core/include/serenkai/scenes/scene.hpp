#pragma once

namespace serenkai {
class Renderer;

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
    virtual void render(Renderer& renderer) = 0;

    // virtual bool handle_event(const Event& e) = 0;

    /// @brief Called once when entering the scene.
    virtual void on_enter() {}

    /// @brief Called once when leaving the scene, used to clean up resources.
    virtual void on_leave() {}

protected:
    /*
        virtual bool handle_mouse_move_event(const MouseMoveEvent&) {
            return false;
        }
        virtual bool handle_mouse_button_event(const MouseButtonEvent&) {
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
            */
};
} // namespace serenkai
