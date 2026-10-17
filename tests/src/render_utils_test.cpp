#include "serenkai/render/render_utils.hpp"

#include <SDL3/SDL_rect.h>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace serenkai;
using Catch::Approx;

TEST_CASE("resolve_flip_transform identity and orthogonal flips",
          "[render][render_utils]") {
    SECTION("FlipFlag::None leaves dst untouched and returns no transform") {
        SDL_FRect dst{10.0f, 20.0f, 30.0f, 40.0f};
        auto [angle, flip_mode] = resolve_flip_transform(FlipFlag::None, dst);
        CHECK(angle == 0.0);
        CHECK(flip_mode == SDL_FLIP_NONE);
        CHECK(dst.x == 10.0f);
        CHECK(dst.y == 20.0f);
        CHECK(dst.w == 30.0f);
        CHECK(dst.h == 40.0f);
    }

    SECTION("Horizontal flip sets SDL_FLIP_HORIZONTAL") {
        SDL_FRect dst{10.0f, 20.0f, 30.0f, 40.0f};
        auto [angle, flip_mode] =
            resolve_flip_transform(FlipFlag::Horizontal, dst);
        CHECK(angle == 0.0);
        CHECK(flip_mode == SDL_FLIP_HORIZONTAL);
        CHECK(dst.x == 10.0f);
        CHECK(dst.y == 20.0f);
        CHECK(dst.w == 30.0f);
        CHECK(dst.h == 40.0f);
    }

    SECTION("Vertical flip sets SDL_FLIP_VERTICAL") {
        SDL_FRect dst{10.0f, 20.0f, 30.0f, 40.0f};
        auto [angle, flip_mode] =
            resolve_flip_transform(FlipFlag::Vertical, dst);
        CHECK(angle == 0.0);
        CHECK(flip_mode == SDL_FLIP_VERTICAL);
        CHECK(dst.x == 10.0f);
        CHECK(dst.y == 20.0f);
        CHECK(dst.w == 30.0f);
        CHECK(dst.h == 40.0f);
    }

    SECTION("Horizontal and Vertical flip sets both flags") {
        SDL_FRect dst{10.0f, 20.0f, 30.0f, 40.0f};
        auto [angle, flip_mode] = resolve_flip_transform(
            FlipFlag::Horizontal | FlipFlag::Vertical, dst);
        CHECK(angle == 0.0);
        CHECK(flip_mode == static_cast<SDL_FlipMode>(SDL_FLIP_HORIZONTAL |
                                                     SDL_FLIP_VERTICAL));
        CHECK(dst.x == 10.0f);
        CHECK(dst.y == 20.0f);
        CHECK(dst.w == 30.0f);
        CHECK(dst.h == 40.0f);
    }
}

TEST_CASE("resolve_flip_transform diagonal flips and center invariance",
          "[render][render_utils]") {
    const SDL_FRect initial_dst{10.0f, 20.0f, 30.0f, 50.0f};
    const float expected_cx = initial_dst.x + initial_dst.w * 0.5f; // 25.0f
    const float expected_cy = initial_dst.y + initial_dst.h * 0.5f; // 45.0f

    auto verify_center = [&](const SDL_FRect& dst) {
        CHECK(dst.w == 50.0f);
        CHECK(dst.h == 30.0f);
        const float cx = dst.x + dst.w * 0.5f;
        const float cy = dst.y + dst.h * 0.5f;
        CHECK(cx == Approx(expected_cx));
        CHECK(cy == Approx(expected_cy));
    };

    SECTION("Diagonal only: 90 deg + vertical flip") {
        SDL_FRect dst = initial_dst;
        auto [angle, flip_mode] =
            resolve_flip_transform(FlipFlag::Diagonal, dst);
        CHECK(angle == 90.0);
        CHECK(flip_mode == SDL_FLIP_VERTICAL);
        verify_center(dst);
    }

    SECTION("Diagonal + Horizontal: 90 deg clockwise (Tiled standard)") {
        SDL_FRect dst = initial_dst;
        auto [angle, flip_mode] = resolve_flip_transform(
            FlipFlag::Diagonal | FlipFlag::Horizontal, dst);
        CHECK(angle == 90.0);
        CHECK(flip_mode == SDL_FLIP_NONE);
        verify_center(dst);
    }

    SECTION("Diagonal + Vertical: 270 deg clockwise") {
        SDL_FRect dst = initial_dst;
        auto [angle, flip_mode] = resolve_flip_transform(
            FlipFlag::Diagonal | FlipFlag::Vertical, dst);
        CHECK(angle == 270.0);
        CHECK(flip_mode == SDL_FLIP_NONE);
        verify_center(dst);
    }

    SECTION("Diagonal + Horizontal + Vertical: 90 deg + horizontal flip") {
        SDL_FRect dst = initial_dst;
        auto [angle, flip_mode] = resolve_flip_transform(
            FlipFlag::Diagonal | FlipFlag::Horizontal | FlipFlag::Vertical,
            dst);
        CHECK(angle == 90.0);
        CHECK(flip_mode == SDL_FLIP_HORIZONTAL);
        verify_center(dst);
    }
}
