#include "serenkai/application/application.hpp"

#include <SDL3/SDL.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace serenkai;

TEST_CASE("DeltaTime calculation", "[application]") {
    Application::DeltaTime delta_time;
    delta_time.last_tick_ns = 1'000'000'000;    // 1.0 second
    delta_time.current_tick_ns = 1'016'000'000; // 1.016 second

    //  16,000,000 ns = 0.016 s
    CHECK(delta_time.dt() == Catch::Approx(0.016));
}

TEST_CASE("Application lifecycle and event handling", "[application]") {

    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");

    char arg0[] = "serenkai_tests";
    char* argv[] = {arg0};
    int argc = 1;

    SECTION("Application initializes in running state") {
        Application app{argc, argv};
        REQUIRE(app.is_running());

        app.step(0.016);
        CHECK(app.is_running());
    }

    SECTION("Application quits on SDL_EVENT_QUIT event") {
        Application app{argc, argv};
        REQUIRE(app.is_running());

        SDL_Event quit_event{};
        quit_event.type = SDL_EVENT_QUIT;
        SDL_PushEvent(&quit_event);

        app.step(0.016);

        CHECK_FALSE(app.is_running());
    }
}

TEST_CASE("Application command line arguments", "[application]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");

    SECTION("Application handles --help by throwing ExitException") {
        char arg0[] = "serenkai_tests";
        char arg1[] = "--help";
        char* argv[] = {arg0, arg1};
        int argc = 2;

        try {
            Application app{argc, argv};
            FAIL("Expected ExitException");
        } catch (const Application::ExitException& e) {
            CHECK(e.code == 0);
        }
    }

    SECTION("Application accepts extra asset directory via --add") {
        char arg0[] = "serenkai_tests";
        char arg1[] = "--add";
        char arg2[] = "./test_assets";
        char* argv[] = {arg0, arg1, arg2};
        int argc = 3;

        Application app{argc, argv};
        CHECK(app.is_running());
    }
}