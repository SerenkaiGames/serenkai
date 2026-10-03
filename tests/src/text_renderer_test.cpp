#include "serenkai/render/text_renderer.hpp"

#include <SDL3/SDL.h>
#include <catch2/catch_test_macros.hpp>

using namespace serenkai;

TEST_CASE("TextRenderer lifecycle and cache operations",
          "[render][text_renderer]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    REQUIRE(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window* window = SDL_CreateWindow("Test", 64, 64, 0);
    REQUIRE(window != nullptr);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    REQUIRE(renderer != nullptr);

    SECTION("Creation and cache clearance") {
        TextRenderer tr(renderer);
        // Clearing an empty cache should be safe
        tr.clear_cache();
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
