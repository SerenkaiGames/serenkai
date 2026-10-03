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

TEST_CASE("RaiiWrapper basic lifecycle and value access", "[raii]") {
    bool init_called = false;
    bool cleanup_called = false;
    int cleaned_value = -1;

    {
        auto init = [&init_called](int& val) {
            val = 42;
            init_called = true;
        };
        auto cleanup = [&cleanup_called, &cleaned_value](int& val) {
            cleanup_called = true;
            cleaned_value = val;
        };

        auto wrapper = make_raii<int>(init, cleanup);
        CHECK(init_called);
        CHECK_FALSE(cleanup_called);
        CHECK(wrapper.get() == 42);
        CHECK(*wrapper == 42);
        CHECK(static_cast<bool>(wrapper));

        *wrapper = 100;
        CHECK(wrapper.get() == 100);
    }

    CHECK(cleanup_called);
    CHECK(cleaned_value == 100);
}

TEST_CASE("RaiiWrapper manual reset and release", "[raii]") {
    int cleanup_count = 0;
    auto init = [](int& val) { val = 10; };
    auto cleanup = [&cleanup_count](int&) { ++cleanup_count; };

    SECTION("reset executes cleanup and deactivates wrapper") {
        auto wrapper = make_raii<int>(init, cleanup);
        CHECK(static_cast<bool>(wrapper));
        wrapper.reset();
        CHECK_FALSE(static_cast<bool>(wrapper));
        CHECK(cleanup_count == 1);

        // Subsequent resets are safe no-ops.
        wrapper.reset();
        CHECK(cleanup_count == 1);
    }

    SECTION("release disables cleanup") {
        {
            auto wrapper = make_raii<int>(init, cleanup);
            wrapper.release();
            CHECK_FALSE(static_cast<bool>(wrapper));
        }
        CHECK(cleanup_count == 0);
    }
}

TEST_CASE("RaiiWrapper move semantics", "[raii]") {
    int cleanup_count = 0;
    int last_cleaned_value = 0;
    auto cleanup = [&cleanup_count, &last_cleaned_value](int& val) {
        ++cleanup_count;
        last_cleaned_value = val;
    };

    SECTION("Move constructor transfers value and ownership") {
        {
            auto init = [](int& val) { val = 42; };
            auto w1 = make_raii<int>(init, cleanup);
            auto w2 = std::move(w1);

            CHECK_FALSE(static_cast<bool>(w1));
            CHECK(static_cast<bool>(w2));
            CHECK(w2.get() == 42);
            CHECK(cleanup_count == 0);
        }
        CHECK(cleanup_count == 1);
        CHECK(last_cleaned_value == 42);
    }

    SECTION("Move assignment cleans up existing target") {
        using Wrapper = RaiiWrapper<int, std::function<void(int&)>,
                                    std::function<void(int&)>>;
        {
            Wrapper target([](int& val) { val = 1; }, cleanup);
            *target = 10;
            Wrapper source([](int& val) { val = 2; }, cleanup);
            *source = 20;

            target = std::move(source);
            CHECK(cleanup_count == 1);
            CHECK(last_cleaned_value == 10);
            CHECK(target.get() == 20);
            CHECK_FALSE(static_cast<bool>(source));
        }
        CHECK(cleanup_count == 2);
        CHECK(last_cleaned_value == 20);
    }
}
