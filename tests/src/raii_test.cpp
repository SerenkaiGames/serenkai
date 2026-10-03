#include "serenkai/base/raii.hpp"

#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <utility>

using namespace serenkai;

TEST_CASE("RaiiGuard basic lifecycle", "[raii]") {
    bool init_called = false;
    bool cleanup_called = false;

    {
        RaiiGuard guard([&init_called]() { init_called = true; },
                        [&cleanup_called]() { cleanup_called = true; });

        CHECK(init_called);
        CHECK_FALSE(cleanup_called);
    }

    CHECK(cleanup_called);
}

TEST_CASE("RaiiGuard manual reset", "[raii]") {
    int cleanup_count = 0;

    {
        RaiiGuard guard([]() {}, [&cleanup_count]() { ++cleanup_count; });

        CHECK(cleanup_count == 0);
        guard.reset();
        CHECK(cleanup_count == 1);

        // Subsequent resets should be safe no-ops.
        guard.reset();
        CHECK(cleanup_count == 1);
    }

    // Destructor should not clean up again after reset.
    CHECK(cleanup_count == 1);
}

TEST_CASE("RaiiGuard release disables cleanup", "[raii]") {
    bool cleanup_called = false;

    {
        RaiiGuard guard([]() {},
                        [&cleanup_called]() { cleanup_called = true; });

        guard.release();
    }

    CHECK_FALSE(cleanup_called);
}

TEST_CASE("RaiiGuard move semantics", "[raii]") {
    SECTION("Move constructor transfers cleanup responsibility") {
        int cleanup_count = 0;

        {
            RaiiGuard guard1([]() {}, [&cleanup_count]() { ++cleanup_count; });

            RaiiGuard guard2(std::move(guard1));
            CHECK(cleanup_count == 0);
        }

        CHECK(cleanup_count == 1);
    }

    SECTION(
        "Move assignment cleans up previous target and transfers ownership") {
        int first_cleanup_count = 0;
        int second_cleanup_count = 0;

        using Guard = RaiiGuard<std::function<void()>, std::function<void()>>;

        {
            Guard target([]() {},
                         [&first_cleanup_count]() { ++first_cleanup_count; });
            Guard source([]() {},
                         [&second_cleanup_count]() { ++second_cleanup_count; });

            CHECK(first_cleanup_count == 0);
            CHECK(second_cleanup_count == 0);

            target = std::move(source);

            // Existing target active cleanup is executed on move-assignment.
            CHECK(first_cleanup_count == 1);
            CHECK(second_cleanup_count == 0);
        }

        CHECK(first_cleanup_count == 1);
        CHECK(second_cleanup_count == 1);
    }
}
