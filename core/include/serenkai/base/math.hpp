#pragma once

#include <cstdint>
namespace serenkai {

inline std::uint32_t combine32(std::uint32_t seed,
                               std::uint32_t hash) noexcept {

    seed ^= hash + 0x9e3779b9u + (seed << 6) + (seed >> 2);
    return seed;
}

inline std::uint32_t fmix32(std::uint32_t h) noexcept {
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

inline std::uint32_t hash_to_32(std::size_t h) noexcept {
    std::uint32_t x = static_cast<std::uint32_t>(h);
    if constexpr (sizeof(std::size_t) > 4) {
        x ^= static_cast<std::uint32_t>(h >> 32);
    }
    return x;
}
} // namespace serenkai