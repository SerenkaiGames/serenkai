#include "serenkai/base/raii.hpp"
#include "serenkai/render/text_renderer.hpp"
#include "serenkai/resource/font.hpp"

#include <SDL3/SDL.h>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <ft2build.h>
#include FT_FREETYPE_H

namespace fs = std::filesystem;
using namespace serenkai;

namespace {

fs::path get_test_font_path() {
#ifdef SERENKAI_TEST_ASSET_DIR
    fs::path defined_path =
        fs::path(SERENKAI_TEST_ASSET_DIR) / "fonts" / "unifont_t-17.0.05.otf";
    if (fs::exists(defined_path)) {
        return defined_path;
    }
#endif
    if (fs::exists("assets/fonts/unifont_t-17.0.05.otf")) {
        return "assets/fonts/unifont_t-17.0.05.otf";
    }
    if (fs::exists("../../assets/fonts/unifont_t-17.0.05.otf")) {
        return "../../assets/fonts/unifont_t-17.0.05.otf";
    }
    return {};
}

} // namespace

TEST_CASE("TextRenderer lifecycle and cache operations",
          "[render][text_renderer]") {
    SDL_SetHint(SDL_HINT_VIDEO_DRIVER, "dummy");
    REQUIRE(SDL_Init(SDL_INIT_VIDEO));
    RaiiGuard sdl_guard([]() {}, []() { SDL_Quit(); });

    SDL_Window* window = SDL_CreateWindow("Test", 64, 64, 0);
    REQUIRE(window != nullptr);
    RaiiGuard window_guard([]() {},
                           [&window]() {
                               if (window) {
                                   SDL_DestroyWindow(window);
                               }
                           });

    SDL_Renderer* renderer = SDL_CreateRenderer(window, nullptr);
    REQUIRE(renderer != nullptr);
    RaiiGuard renderer_guard([]() {},
                             [&renderer]() {
                                 if (renderer) {
                                     SDL_DestroyRenderer(renderer);
                                 }
                             });

    SECTION("Creation and cache clearance") {
        TextRenderer tr(renderer);
        tr.clear_cache();
    }

    SECTION("Measurement and rendering with real font") {
        fs::path font_path = get_test_font_path();
        REQUIRE_FALSE(font_path.empty());

        FT_Library lib = nullptr;
        REQUIRE(FT_Init_FreeType(&lib) == 0);
        RaiiGuard lib_guard([]() {}, [&lib]() { FT_Done_FreeType(lib); });

        Font font(font_path.string(), 16, lib);
        TextRenderer tr(renderer);

        // Measurement assertions
        CHECK(tr.measure_width(font, "") == 0);
        int width_single = tr.measure_width(font, "A");
        int width_multi = tr.measure_width(font, "ABCD");
        CHECK(width_single > 0);
        CHECK(width_multi > width_single);

        // Rendering empty string
        tr.draw_text(font, "", 0, 0, {255, 255, 255, 255});

        // Initial rendering populates texture cache
        tr.draw_text(font, "Hello", 0, 0, {255, 255, 255, 255});

        // Subsequent rendering hits cached textures
        tr.draw_text(font, "Hello", 10, 10, {255, 0, 0, 128});

        // Clearing cache and re-rendering
        tr.clear_cache();
        tr.draw_text(font, "Hello", 0, 0, {0, 255, 0, 255});
    }
}
