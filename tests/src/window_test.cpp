#include "serenkai/application/window_manager.hpp"

#include <SDL3/SDL.h>
#include <catch2/catch_test_macros.hpp>

TEST_CASE("WindowManager headless initialization and destruction", "[window]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    REQUIRE(SDL_Init(SDL_INIT_VIDEO));

    SECTION("Default window configuration") {
        WindowConfig config{};
        WindowManager wm(config);
        REQUIRE(wm.get_window() != nullptr);
    }

    SECTION("Custom window dimensions") {
        WindowConfig config{.width = 1920, .height = 1080};
        WindowManager wm(config);
        REQUIRE(wm.get_window() != nullptr);
    }

    SDL_Quit();
}

TEST_CASE("WindowManager fullscreen mode transitions", "[window]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    REQUIRE(SDL_Init(SDL_INIT_VIDEO));

    WindowConfig config{};
    WindowManager wm(config);

    SECTION("Switching fullscreen modes") {

        CHECK(wm.set_fullscreen(FullscreenMode::Fullscreen));
        CHECK(wm.set_fullscreen(FullscreenMode::Windowed));
        CHECK(wm.set_fullscreen(FullscreenMode::FullscreenBorderless));
        CHECK(wm.set_fullscreen(FullscreenMode::Windowed));
    }

    SDL_Quit();
}