#pragma once
#include <cstdint>
namespace serenkai {

constexpr std::uint32_t combine32(std::uint32_t seed,
                                  std::uint32_t hash) noexcept {

    seed ^= hash + 0x9e3779b9u + (seed << 6) + (seed >> 2);
    return seed;
}

constexpr std::uint32_t fmix32(std::uint32_t h) noexcept {
    h ^= h >> 16;
    h *= 0x85ebca6bu;
    h ^= h >> 13;
    h *= 0xc2b2ae35u;
    h ^= h >> 16;
    return h;
}

constexpr std::uint32_t hash_to_32(std::uint64_t h) noexcept {
    h ^= h >> 33;
    h *= 0xff51afd7ed558ccdULL;
    h ^= h >> 33;
    h *= 0xc4ceb9fe1a85ec53ULL;
    h ^= h >> 33;
    return static_cast<std::uint32_t>(h);
}
} // namespace serenkai