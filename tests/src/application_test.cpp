#include "serenkai/application/application.hpp"

#include <SDL3/SDL.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("DeltaTime calculation", "[application]") {
    Application::DeltaTime delta_time;
    delta_time.last_tick_ns = 1'000'000'000;    // 1.0 second
    delta_time.current_tick_ns = 1'016'000'000; // 1.016 second

    //  16,000,000 ns = 0.016 s
    CHECK(delta_time.dt() == Catch::Approx(0.016));
}

TEST_CASE("Application lifecycle and event handling", "[application]") {

    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");

    SECTION("Application initializes in running state") {
        Application app;
        REQUIRE(app.is_running());

        app.step(0.016);
        CHECK(app.is_running());
    }

    SECTION("Application quits on SDL_EVENT_QUIT event") {
        Application app;
        REQUIRE(app.is_running());

        SDL_Event quit_event{};
        quit_event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quit_event);

        app.step(0.016);

        CHECK_FALSE(app.is_running());
    }
}