#pragma once

namespace serenkai {

/// @brief Enumeration of mouse buttons and keyboard keys.
enum class Key {
    Unknown,

    // Letter keys
    A,
    B,
    C,
    D,
    E,
    F,
    G,
    H,
    I,
    J,
    K,
    L,
    M,
    N,
    O,
    P,
    Q,
    R,
    S,
    T,
    U,
    V,
    W,
    X,
    Y,
    Z,

    // Digit keys (main keyboard area)
    Digit0,
    Digit1,
    Digit2,
    Digit3,
    Digit4,
    Digit5,
    Digit6,
    Digit7,
    Digit8,
    Digit9,

    // Function keys
    F1,
    F2,
    F3,
    F4,
    F5,
    F6,
    F7,
    F8,
    F9,
    F10,
    F11,
    F12,

    // Control keys
    Backspace,
    Tab,
    Enter,
    Escape,
    Space,
    CapsLock,
    NumLock,
    ScrollLock,

    // Modifier keys
    LeftShift,
    RightShift,
    LeftCtrl,
    RightCtrl,
    LeftAlt,
    RightAlt,
    LeftSuper,
    RightSuper, // Windows / Command key

    // Navigation keys
    Insert,
    Delete,
    Home,
    End,
    PageUp,
    PageDown,
    Left,
    Right,
    Up,
    Down,

    // Lock and system keys
    PrintScreen,
    Pause,

    // Main keyboard area symbol keys
    GraveAccent,  // `
    Minus,        // -
    Equals,       // =
    LeftBracket,  // [
    RightBracket, // ]
    Backslash,    // \ /
    Semicolon,    // ;
    Apostrophe,   // '
    Comma,        // ,
    Period,       // .
    Slash,        // /

    // Numpad area
    Numpad0,
    Numpad1,
    Numpad2,
    Numpad3,
    Numpad4,
    Numpad5,
    Numpad6,
    Numpad7,
    Numpad8,
    Numpad9,
    NumpadAdd,      // +
    NumpadSubtract, // -
    NumpadMultiply, // *
    NumpadDivide,   // /
    NumpadDecimal,  // .
    NumpadEnter,

    // mouse
    MouseLeft,
    MouseRight,
    MouseMiddle,

    MouseBack,    // Side button back
    MouseForward, // Side button forward
};

/// @brief Enumeration of key actions.
enum class KeyAction { Press, Release, Repeat };
} // namespace serenkai