#pragma once
#include "serenkai/base/unreachable.hpp"

#include <SDL3/SDL_pixels.h>
#include <glm/ext/vector_float4.hpp>
namespace serenkai {
/// @brief Color enumeration
///
/// Contains only the three RGB channels, and does not include alpha.
enum class Color {
    Black = 0,
    White,
    Red,
    Green,
    Blue,
    Yellow,
    Cyan,
    Magenta,
    Gray,
    Orange,
    Purple,
    Pink,
    Brown
};

/// @brief Returns the color value as a floating-point number.
inline constexpr glm::vec4 color_value(Color color) {
    using glm::vec4;

    switch (color) {
    case Color::Black:
        return vec4{0.0f, 0.0f, 0.0f, 1.0f};
    case Color::White:
        return vec4{1.0f, 1.0f, 1.0f, 1.0f};
    case Color::Red:
        return vec4{1.0f, 0.0f, 0.0f, 1.0f};
    case Color::Green:
        return vec4{0.0f, 1.0f, 0.0f, 1.0f};
    case Color::Blue:
        return vec4{0.0f, 0.0f, 1.0f, 1.0f};
    case Color::Yellow:
        return vec4{1.0f, 1.0f, 0.0f, 1.0f};
    case Color::Cyan:
        return vec4{0.0f, 1.0f, 1.0f, 1.0f};
    case Color::Magenta:
        return vec4{1.0f, 0.0f, 1.0f, 1.0f};
    case Color::Gray:
        return vec4{0.5f, 0.5f, 0.5f, 1.0f};
    case Color::Orange:
        return vec4{1.0f, 0.647f, 0.0f, 1.0f};
    case Color::Purple:
        return vec4{0.502f, 0.0f, 0.502f, 1.0f};
    case Color::Pink:
        return vec4{1.0f, 0.753f, 0.769f, 1.0f};
    case Color::Brown:
        return vec4{0.647f, 0.165f, 0.165f, 1.0f};
    default:
        unreachable("Unknown color enum");
    }
}

inline constexpr SDL_FColor to_sdl_fcolor(Color color) {
    auto value = color_value(color);
    return SDL_FColor{value.r, value.g, value.b, value.a};
}

} // namespace serenkai
