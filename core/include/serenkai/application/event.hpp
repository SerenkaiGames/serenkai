#pragma once

#include "serenkai/application/key.hpp"

#include <string>
#include <variant>

namespace serenkai {

struct MouseMoveEvent {
    float xpos;
    float ypos;
    float xrel;
    float yrel;
    MouseMoveEvent(float x, float y, float dx, float dy)
        : xpos(x), ypos(y), xrel(dx), yrel(dy) {}
};

struct MouseWheelEvent {
    float xoffset;
    float yoffset;

    explicit MouseWheelEvent(float x, float y) : xoffset(x), yoffset(y) {}
};

struct KeyEvent {
    Key key;
    KeyAction action;

    explicit KeyEvent(Key k, KeyAction a) : key(k), action(a) {}
};

struct TextInputEvent {
    std::string text;

    explicit TextInputEvent(std::string t) : text(std::move(t)) {}
};

struct WindowResizeEvent {
    int width;
    int height;

    explicit WindowResizeEvent(int w, int h) : width(w), height(h) {}
};

struct FrameBufferResizeEvent {
    int width;
    int height;

    explicit FrameBufferResizeEvent(int w, int h) : width(w), height(h) {}
};

struct WindowFocusChangeEvent {
    bool focus;

    explicit WindowFocusChangeEvent(bool f) : focus(f) {}
};

struct QuitEvent {};

using Event =
    std::variant<MouseMoveEvent, MouseWheelEvent, KeyEvent, TextInputEvent,
                 WindowResizeEvent, FrameBufferResizeEvent, QuitEvent,
                 WindowFocusChangeEvent>;

template <class... T> struct Overloaded : T... {
    using T::operator()...;
};
template <class... T> Overloaded(T...) -> Overloaded<T...>;

} // namespace serenkai
