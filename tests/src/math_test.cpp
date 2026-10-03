#include "serenkai/base/math.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cstdint>

using namespace serenkai;

TEST_CASE("combine32 properties and determinism", "[math]") {
    SECTION("Deterministic output for fixed inputs") {
        uint32_t seed = 0;
        uint32_t h1 = combine32(seed, 42);
        uint32_t h2 = combine32(seed, 42);
        CHECK(h1 == h2);
    }

    SECTION("Order of inputs affects the combined result") {
        uint32_t seed = 0;
        uint32_t ab = combine32(combine32(seed, 1), 2);
        uint32_t ba = combine32(combine32(seed, 2), 1);
        CHECK(ab != ba);
    }

    SECTION("Different seeds produce different results") {
        uint32_t h1 = combine32(100, 42);
        uint32_t h2 = combine32(200, 42);
        CHECK(h1 != h2);
    }
}

TEST_CASE("fmix32 avalanche and mapping", "[math]") {
    SECTION("Zero maps to zero") { CHECK(fmix32(0) == 0); }

    SECTION("Non-zero values are deterministic") {
        uint32_t val = 0x12345678;
        CHECK(fmix32(val) == fmix32(val));
        CHECK(fmix32(val) != 0);
    }

    SECTION("Different inputs produce distinct outputs") {
        CHECK(fmix32(1) != fmix32(2));
        CHECK(fmix32(0xffffffff) != fmix32(0x7fffffff));
    }
}

TEST_CASE("hash_to_32 size folding", "[math]") {
    SECTION("32-bit values preserve lower bits") {
        std::size_t val = 0x12345678u;
        CHECK(hash_to_32(val) == 0x12345678u);
    }

    if constexpr (sizeof(std::size_t) > 4) {
        SECTION("64-bit values fold upper and lower halves") {
            std::size_t upper = 0xaaaaaaaau;
            std::size_t lower = 0x55555555u;
            std::size_t combined = (upper << 32) | lower;
            CHECK(hash_to_32(combined) == (0xaaaaaaaau ^ 0x55555555u));
        }
    }
}
