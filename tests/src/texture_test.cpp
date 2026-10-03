#include "serenkai/render/texture.hpp"

#include <SDL3/SDL.h>
#include <catch2/catch_test_macros.hpp>

using namespace serenkai;

TEST_CASE("Texture default state", "[render][texture]") {
    Texture tex;
    CHECK(tex.get() == nullptr);
    CHECK_FALSE(static_cast<bool>(tex));
    CHECK(tex.release() == nullptr);
}

TEST_CASE("Texture lifecycle and ownership transfer", "[render][texture]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    REQUIRE(SDL_Init(SDL_INIT_VIDEO));

    SDL_Window* window = SDL_CreateWindow("Test", 64, 64, 0);
    REQUIRE(window != nullptr);

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    REQUIRE(renderer != nullptr);

    SECTION("Constructor wraps raw SDL_Texture") {
        SDL_Texture* raw = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STATIC, 16, 16);
        REQUIRE(raw != nullptr);

        {
            Texture tex(raw);
            CHECK(tex.get() == raw);
            CHECK(static_cast<bool>(tex));
        }
    }

    SECTION("Move constructor transfers ownership") {
        SDL_Texture* raw = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STATIC, 16, 16);
        REQUIRE(raw != nullptr);

        Texture tex1(raw);
        Texture tex2(std::move(tex1));

        CHECK(tex1.get() == nullptr);
        CHECK_FALSE(static_cast<bool>(tex1));
        CHECK(tex2.get() == raw);
        CHECK(static_cast<bool>(tex2));
    }

    SECTION("Move assignment transfers ownership and destroys previous") {
        SDL_Texture* raw1 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                              SDL_TEXTUREACCESS_STATIC, 16, 16);
        SDL_Texture* raw2 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                              SDL_TEXTUREACCESS_STATIC, 16, 16);
        REQUIRE(raw1 != nullptr);
        REQUIRE(raw2 != nullptr);

        Texture tex1(raw1);
        Texture tex2(raw2);

        tex2 = std::move(tex1);

        CHECK(tex1.get() == nullptr);
        CHECK(tex2.get() == raw1);
    }

    SECTION("Release relinquishes ownership without destruction") {
        SDL_Texture* raw = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                             SDL_TEXTUREACCESS_STATIC, 16, 16);
        REQUIRE(raw != nullptr);

        Texture tex(raw);
        SDL_Texture* released = tex.release();

        CHECK(released == raw);
        CHECK(tex.get() == nullptr);
        CHECK_FALSE(static_cast<bool>(tex));

        SDL_DestroyTexture(released);
    }

    SECTION("Reset replaces managed texture and handles self-reset") {
        SDL_Texture* raw1 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                              SDL_TEXTUREACCESS_STATIC, 16, 16);
        SDL_Texture* raw2 = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                                              SDL_TEXTUREACCESS_STATIC, 16, 16);
        REQUIRE(raw1 != nullptr);
        REQUIRE(raw2 != nullptr);

        Texture tex(raw1);

        // Self-reset should be a safe no-op
        tex.reset(raw1);
        CHECK(tex.get() == raw1);

        tex.reset(raw2);
        CHECK(tex.get() == raw2);

        tex.reset(nullptr);
        CHECK(tex.get() == nullptr);
        CHECK_FALSE(static_cast<bool>(tex));
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
}
