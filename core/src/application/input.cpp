#include "serenkai/application/input.hpp"

#include "serenkai/application/event.hpp"
#include "serenkai/application/key.hpp"
#include "serenkai/base/assert.hpp"

#include <optional>
#include <spdlog/spdlog.h>

namespace serenkai::input {

namespace {
inline std::optional<Event> handle_sdl_key(const SDL_Event& e) {

    SE_ASSERT(e.type == SDL_EVENT_KEY_DOWN || e.type == SDL_EVENT_KEY_UP);

    Key key = Key::Unknown;

    switch (e.key.key) {
    // Letter keys
    case SDLK_A:
        key = Key::A;
        break;
    case SDLK_B:
        key = Key::B;
        break;
    case SDLK_C:
        key = Key::C;
        break;
    case SDLK_D:
        key = Key::D;
        break;
    case SDLK_E:
        key = Key::E;
        break;
    case SDLK_F:
        key = Key::F;
        break;
    case SDLK_G:
        key = Key::G;
        break;
    case SDLK_H:
        key = Key::H;
        break;
    case SDLK_I:
        key = Key::I;
        break;
    case SDLK_J:
        key = Key::J;
        break;
    case SDLK_K:
        key = Key::K;
        break;
    case SDLK_L:
        key = Key::L;
        break;
    case SDLK_M:
        key = Key::M;
        break;
    case SDLK_N:
        key = Key::N;
        break;
    case SDLK_O:
        key = Key::O;
        break;
    case SDLK_P:
        key = Key::P;
        break;
    case SDLK_Q:
        key = Key::Q;
        break;
    case SDLK_R:
        key = Key::R;
        break;
    case SDLK_S:
        key = Key::S;
        break;
    case SDLK_T:
        key = Key::T;
        break;
    case SDLK_U:
        key = Key::U;
        break;
    case SDLK_V:
        key = Key::V;
        break;
    case SDLK_W:
        key = Key::W;
        break;
    case SDLK_X:
        key = Key::X;
        break;
    case SDLK_Y:
        key = Key::Y;
        break;
    case SDLK_Z:
        key = Key::Z;
        break;

    // Digit keys
    case SDLK_0:
        key = Key::Digit0;
        break;
    case SDLK_1:
        key = Key::Digit1;
        break;
    case SDLK_2:
        key = Key::Digit2;
        break;
    case SDLK_3:
        key = Key::Digit3;
        break;
    case SDLK_4:
        key = Key::Digit4;
        break;
    case SDLK_5:
        key = Key::Digit5;
        break;
    case SDLK_6:
        key = Key::Digit6;
        break;
    case SDLK_7:
        key = Key::Digit7;
        break;
    case SDLK_8:
        key = Key::Digit8;
        break;
    case SDLK_9:
        key = Key::Digit9;
        break;

    // Function keys
    case SDLK_F1:
        key = Key::F1;
        break;
    case SDLK_F2:
        key = Key::F2;
        break;
    case SDLK_F3:
        key = Key::F3;
        break;
    case SDLK_F4:
        key = Key::F4;
        break;
    case SDLK_F5:
        key = Key::F5;
        break;
    case SDLK_F6:
        key = Key::F6;
        break;
    case SDLK_F7:
        key = Key::F7;
        break;
    case SDLK_F8:
        key = Key::F8;
        break;
    case SDLK_F9:
        key = Key::F9;
        break;
    case SDLK_F10:
        key = Key::F10;
        break;
    case SDLK_F11:
        key = Key::F11;
        break;
    case SDLK_F12:
        key = Key::F12;
        break;

    // Control keys
    case SDLK_BACKSPACE:
        key = Key::Backspace;
        break;
    case SDLK_TAB:
        key = Key::Tab;
        break;
    case SDLK_RETURN:
        key = Key::Enter;
        break;
    case SDLK_ESCAPE:
        key = Key::Escape;
        break;
    case SDLK_SPACE:
        key = Key::Space;
        break;
    case SDLK_CAPSLOCK:
        key = Key::CapsLock;
        break;
    case SDLK_NUMLOCKCLEAR:
        key = Key::NumLock;
        break;
    case SDLK_SCROLLLOCK:
        key = Key::ScrollLock;
        break;

    // Modifier keys
    case SDLK_LSHIFT:
        key = Key::LeftShift;
        break;
    case SDLK_RSHIFT:
        key = Key::RightShift;
        break;
    case SDLK_LCTRL:
        key = Key::LeftCtrl;
        break;
    case SDLK_RCTRL:
        key = Key::RightCtrl;
        break;
    case SDLK_LALT:
        key = Key::LeftAlt;
        break;
    case SDLK_RALT:
        key = Key::RightAlt;
        break;
    case SDLK_LGUI:
        key = Key::LeftSuper;
        break;
    case SDLK_RGUI:
        key = Key::RightSuper;
        break;

    // Navigation keys
    case SDLK_INSERT:
        key = Key::Insert;
        break;
    case SDLK_DELETE:
        key = Key::Delete;
        break;
    case SDLK_HOME:
        key = Key::Home;
        break;
    case SDLK_END:
        key = Key::End;
        break;
    case SDLK_PAGEUP:
        key = Key::PageUp;
        break;
    case SDLK_PAGEDOWN:
        key = Key::PageDown;
        break;

    case SDLK_LEFT:
        key = Key::Left;
        break;
    case SDLK_RIGHT:
        key = Key::Right;
        break;
    case SDLK_UP:
        key = Key::Up;
        break;
    case SDLK_DOWN:
        key = Key::Down;
        break;

    // System keys
    case SDLK_PRINTSCREEN:
        key = Key::PrintScreen;
        break;
    case SDLK_PAUSE:
        key = Key::Pause;
        break;

    // Symbol keys
    case SDLK_GRAVE:
        key = Key::GraveAccent;
        break;
    case SDLK_MINUS:
        key = Key::Minus;
        break;
    case SDLK_EQUALS:
        key = Key::Equals;
        break;
    case SDLK_LEFTBRACKET:
        key = Key::LeftBracket;
        break;
    case SDLK_RIGHTBRACKET:
        key = Key::RightBracket;
        break;
    case SDLK_BACKSLASH:
        key = Key::Backslash;
        break;
    case SDLK_SEMICOLON:
        key = Key::Semicolon;
        break;
    case SDLK_APOSTROPHE:
        key = Key::Apostrophe;
        break;
    case SDLK_COMMA:
        key = Key::Comma;
        break;
    case SDLK_PERIOD:
        key = Key::Period;
        break;
    case SDLK_SLASH:
        key = Key::Slash;
        break;

    // Numpad
    case SDLK_KP_0:
        key = Key::Numpad0;
        break;
    case SDLK_KP_1:
        key = Key::Numpad1;
        break;
    case SDLK_KP_2:
        key = Key::Numpad2;
        break;
    case SDLK_KP_3:
        key = Key::Numpad3;
        break;
    case SDLK_KP_4:
        key = Key::Numpad4;
        break;
    case SDLK_KP_5:
        key = Key::Numpad5;
        break;
    case SDLK_KP_6:
        key = Key::Numpad6;
        break;
    case SDLK_KP_7:
        key = Key::Numpad7;
        break;
    case SDLK_KP_8:
        key = Key::Numpad8;
        break;
    case SDLK_KP_9:
        key = Key::Numpad9;
        break;

    case SDLK_KP_PLUS:
        key = Key::NumpadAdd;
        break;
    case SDLK_KP_MINUS:
        key = Key::NumpadSubtract;
        break;
    case SDLK_KP_MULTIPLY:
        key = Key::NumpadMultiply;
        break;
    case SDLK_KP_DIVIDE:
        key = Key::NumpadDivide;
        break;
    case SDLK_KP_PERIOD:
        key = Key::NumpadDecimal;
        break;
    case SDLK_KP_ENTER:
        key = Key::NumpadEnter;
        break;

    default:
        spdlog::error("Unknown key {}", e.key.key);
        return std::nullopt;
    }

    KeyAction act = KeyAction::Press;

    if (e.type == SDL_EVENT_KEY_DOWN) {
        if (e.key.repeat) {
            act = KeyAction::Repeat;
        } else {
            act = KeyAction::Press;
        }
    } else if (e.type == SDL_EVENT_KEY_UP) {
        act = KeyAction::Release;
    } else {
        spdlog::error("Unknown key event {}", e.type);
        return std::nullopt;
    }

    return KeyEvent{key, act};
}

inline std::optional<Event> handle_sdl_mouse_button(const SDL_Event& e) {
    SE_ASSERT(e.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
              e.type == SDL_EVENT_MOUSE_BUTTON_UP);

    Key key = Key::Unknown;
    KeyAction act = KeyAction::Press;

    switch (e.button.button) {
    case SDL_BUTTON_LEFT:
        key = Key::MouseLeft;
        break;

    case SDL_BUTTON_RIGHT:
        key = Key::MouseRight;
        break;

    case SDL_BUTTON_MIDDLE:
        key = Key::MouseMiddle;
        break;

    case SDL_BUTTON_X1:
        key = Key::MouseBack;
        break;

    case SDL_BUTTON_X2:
        key = Key::MouseForward;
        break;

    default:
        spdlog::error("Unknown mouse button {}", e.button.button);
        return std::nullopt;
    }

    if (e.type == SDL_EVENT_MOUSE_BUTTON_DOWN) {
        act = KeyAction::Press;
    } else if (e.type == SDL_EVENT_MOUSE_BUTTON_UP) {
        act = KeyAction::Release;
    } else {
        spdlog::error("Unknown mouse event type {}", e.type);
        return std::nullopt;
    }

    return KeyEvent{key, act};
}

} // namespace

std::optional<Event> process_sdl_event(const SDL_Event& e) {
    switch (e.type) {
    case SDL_EVENT_QUIT:
        return QuitEvent{};

    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
        return handle_sdl_mouse_button(e);

    case SDL_EVENT_KEY_UP:
    case SDL_EVENT_KEY_DOWN:
        return handle_sdl_key(e);

    case SDL_EVENT_MOUSE_MOTION:

        if (std::abs(e.motion.xrel) > 200 || std::abs(e.motion.yrel) > 200) {
            return std::nullopt;
        }
        return MouseMoveEvent{e.motion.x, e.motion.y, e.motion.xrel,
                              e.motion.yrel};

    case SDL_EVENT_WINDOW_RESIZED:
        return WindowResizeEvent{e.window.data1, e.window.data2};

    case SDL_EVENT_WINDOW_PIXEL_SIZE_CHANGED:
        return FrameBufferResizeEvent{e.window.data1, e.window.data2};

    case SDL_EVENT_MOUSE_WHEEL: {

        float scroll_x = e.wheel.x;
        float scroll_y = e.wheel.y;
        if (e.wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
            scroll_y = -scroll_y;
        }
        return MouseWheelEvent{scroll_x, scroll_y};
    }

    case SDL_EVENT_WINDOW_FOCUS_GAINED:
        return WindowFocusChangeEvent{true};

    case SDL_EVENT_WINDOW_FOCUS_LOST:
        return WindowFocusChangeEvent{false};

    case SDL_EVENT_TEXT_INPUT:
        return TextInputEvent{e.text.text};
    }

    return std::nullopt;
}
} // namespace serenkai::input
