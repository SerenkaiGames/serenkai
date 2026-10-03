#include "serenkai/gui/color.hpp"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_floating_point.hpp>

using namespace serenkai;
using Catch::Matchers::WithinAbs;

TEST_CASE("Color values and SDL_FColor conversion", "[gui][color]") {
    SECTION("Primary colors conversion") {
        auto black = color_value(Color::Black);
        CHECK(black == glm::vec4{0.0f, 0.0f, 0.0f, 1.0f});
        auto sdl_black = to_sdl_fcolor(Color::Black);
        CHECK(sdl_black.r == 0.0f);
        CHECK(sdl_black.g == 0.0f);
        CHECK(sdl_black.b == 0.0f);
        CHECK(sdl_black.a == 1.0f);

        auto white = color_value(Color::White);
        CHECK(white == glm::vec4{1.0f, 1.0f, 1.0f, 1.0f});
        auto sdl_white = to_sdl_fcolor(Color::White);
        CHECK(sdl_white.r == 1.0f);
        CHECK(sdl_white.g == 1.0f);
        CHECK(sdl_white.b == 1.0f);
        CHECK(sdl_white.a == 1.0f);

        auto red = color_value(Color::Red);
        CHECK(red == glm::vec4{1.0f, 0.0f, 0.0f, 1.0f});
        auto green = color_value(Color::Green);
        CHECK(green == glm::vec4{0.0f, 1.0f, 0.0f, 1.0f});
        auto blue = color_value(Color::Blue);
        CHECK(blue == glm::vec4{0.0f, 0.0f, 1.0f, 1.0f});
    }

    SECTION("Secondary and other colors") {
        auto yellow = to_sdl_fcolor(Color::Yellow);
        CHECK(yellow.r == 1.0f);
        CHECK(yellow.g == 1.0f);
        CHECK(yellow.b == 0.0f);
        CHECK(yellow.a == 1.0f);

        auto cyan = to_sdl_fcolor(Color::Cyan);
        CHECK(cyan.r == 0.0f);
        CHECK(cyan.g == 1.0f);
        CHECK(cyan.b == 1.0f);

        auto magenta = to_sdl_fcolor(Color::Magenta);
        CHECK(magenta.r == 1.0f);
        CHECK(magenta.g == 0.0f);
        CHECK(magenta.b == 1.0f);

        auto gray = to_sdl_fcolor(Color::Gray);
        CHECK(gray.r == 0.5f);
        CHECK(gray.g == 0.5f);
        CHECK(gray.b == 0.5f);

        auto orange = to_sdl_fcolor(Color::Orange);
        CHECK_THAT(orange.r, WithinAbs(1.0f, 0.001f));
        CHECK_THAT(orange.g, WithinAbs(0.647f, 0.001f));
        CHECK_THAT(orange.b, WithinAbs(0.0f, 0.001f));

        auto purple = to_sdl_fcolor(Color::Purple);
        CHECK_THAT(purple.r, WithinAbs(0.502f, 0.001f));
        CHECK_THAT(purple.g, WithinAbs(0.0f, 0.001f));
        CHECK_THAT(purple.b, WithinAbs(0.502f, 0.001f));

        auto pink = to_sdl_fcolor(Color::Pink);
        CHECK_THAT(pink.r, WithinAbs(1.0f, 0.001f));
        CHECK_THAT(pink.g, WithinAbs(0.753f, 0.001f));
        CHECK_THAT(pink.b, WithinAbs(0.769f, 0.001f));

        auto brown = to_sdl_fcolor(Color::Brown);
        CHECK_THAT(brown.r, WithinAbs(0.647f, 0.001f));
        CHECK_THAT(brown.g, WithinAbs(0.165f, 0.001f));
        CHECK_THAT(brown.b, WithinAbs(0.165f, 0.001f));
    }
}
