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

TEST_CASE("hash_to_32 mixing and determinism", "[math]") {
    SECTION("Deterministic output for fixed inputs") {
        uint64_t val = 0x123456789abcdef0ULL;
        CHECK(hash_to_32(val) == hash_to_32(val));
    }

    SECTION("Different 64-bit inputs produce distinct outputs") {
        CHECK(hash_to_32(0) != hash_to_32(1));
        CHECK(hash_to_32(0x1234567800000000ULL) !=
              hash_to_32(0x0000000012345678ULL));
    }

    SECTION("Avalanche behavior on single-bit flip") {
        uint64_t val1 = 0x123456789abcdef0ULL;
        uint64_t val2 = val1 ^ 1ULL;
        CHECK(hash_to_32(val1) != hash_to_32(val2));
    }
}
