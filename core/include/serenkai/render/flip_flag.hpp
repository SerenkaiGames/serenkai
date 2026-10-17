#pragma once
#include <cstdint>
namespace serenkai {

enum class FlipFlag : std::uint8_t {
    None = 0,
    Horizontal = 1 << 0,
    Vertical = 1 << 1,
    Diagonal = 1 << 2,
};

constexpr FlipFlag operator|(FlipFlag a, FlipFlag b) noexcept {
    return static_cast<FlipFlag>(static_cast<std::uint8_t>(a) |
                                 static_cast<std::uint8_t>(b));
}

constexpr FlipFlag operator&(FlipFlag a, FlipFlag b) noexcept {
    return static_cast<FlipFlag>(static_cast<std::uint8_t>(a) &
                                 static_cast<std::uint8_t>(b));
}

constexpr FlipFlag& operator|=(FlipFlag& a, FlipFlag b) noexcept {
    a = a | b;
    return a;
}

constexpr FlipFlag operator~(FlipFlag a) noexcept {
    return static_cast<FlipFlag>(~static_cast<std::uint8_t>(a));
}

} // namespace serenkai