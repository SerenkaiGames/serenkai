#include "serenkai/render/sprite_renderer.hpp"

#include <SDL3/SDL.h>
#include <catch2/catch_test_macros.hpp>

using namespace serenkai;

namespace {
struct TestSdlContext {
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;

    TestSdlContext() {
        SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
        if (SDL_Init(SDL_INIT_VIDEO)) {
            window = SDL_CreateWindow("SpriteRendererTest", 64, 64, 0);
            if (window) {
                renderer = SDL_CreateRenderer(window, nullptr);
            }
        }
    }

    ~TestSdlContext() {
        if (renderer) {
            SDL_DestroyRenderer(renderer);
        }
        if (window) {
            SDL_DestroyWindow(window);
        }
        SDL_Quit();
    }

    bool is_valid() const { return window && renderer; }
};
} // namespace

TEST_CASE("SpriteRenderer safety and rendering", "[render][sprite_renderer]") {
    TestSdlContext ctx;
    REQUIRE(ctx.is_valid());

    SECTION("Null renderer is safe") {
        SpriteRenderer null_renderer(nullptr);
        SDL_Surface* surface =
            SDL_CreateSurface(16, 16, SDL_PIXELFORMAT_RGBA32);
        REQUIRE(surface != nullptr);
        SDL_Texture* texture =
            SDL_CreateTextureFromSurface(ctx.renderer, surface);
        REQUIRE(texture != nullptr);

        null_renderer.render_sprite(texture, {0.0f, 0.0f}, {16.0f, 16.0f},
                                    {0.0f, 0.0f}, 1.0f);

        SDL_DestroyTexture(texture);
        SDL_DestroySurface(surface);
    }

    SECTION("Null texture is safe") {
        SpriteRenderer renderer(ctx.renderer);
        renderer.render_sprite(nullptr, {10.0f, 20.0f}, {16.0f, 16.0f},
                               {0.0f, 0.0f}, 1.0f);
    }

    SECTION("Valid texture renders with zoom and offset") {
        SpriteRenderer renderer(ctx.renderer);
        SDL_Surface* surface =
            SDL_CreateSurface(32, 32, SDL_PIXELFORMAT_RGBA32);
        REQUIRE(surface != nullptr);
        SDL_Texture* texture =
            SDL_CreateTextureFromSurface(ctx.renderer, surface);
        REQUIRE(texture != nullptr);

        renderer.render_sprite(texture, {5.0f, 10.0f}, {32.0f, 32.0f},
                               {100.0f, 50.0f}, 2.0f);

        SDL_DestroyTexture(texture);
        SDL_DestroySurface(surface);
    }

    SECTION("Valid texture renders with src_rect") {
        SpriteRenderer renderer(ctx.renderer);
        SDL_Surface* surface =
            SDL_CreateSurface(32, 32, SDL_PIXELFORMAT_RGBA32);
        REQUIRE(surface != nullptr);
        SDL_Texture* texture =
            SDL_CreateTextureFromSurface(ctx.renderer, surface);
        REQUIRE(texture != nullptr);

        renderer.render_sprite(texture, {5.0f, 10.0f}, {16.0f, 16.0f},
                               {100.0f, 50.0f}, 1.0f,
                               glm::vec4{0.0f, 0.0f, 16.0f, 16.0f});

        SDL_DestroyTexture(texture);
        SDL_DestroySurface(surface);
    }

    SECTION("Valid texture renders with flip flags") {
        SpriteRenderer renderer(ctx.renderer);
        SDL_Surface* surface =
            SDL_CreateSurface(32, 32, SDL_PIXELFORMAT_RGBA32);
        REQUIRE(surface != nullptr);
        SDL_Texture* texture =
            SDL_CreateTextureFromSurface(ctx.renderer, surface);
        REQUIRE(texture != nullptr);

        renderer.render_sprite(texture, {5.0f, 10.0f}, {32.0f, 32.0f},
                               {0.0f, 0.0f}, 1.0f, std::nullopt,
                               FlipFlag::Horizontal | FlipFlag::Vertical);

        renderer.render_sprite(
            texture, {5.0f, 10.0f}, {32.0f, 32.0f}, {0.0f, 0.0f}, 1.0f,
            glm::vec4{0.0f, 0.0f, 16.0f, 16.0f}, FlipFlag::Diagonal);

        SDL_DestroyTexture(texture);
        SDL_DestroySurface(surface);
    }
}
