#include "serenkai/base/raii.hpp"
#include "serenkai/resource/font.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdexcept>

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

TEST_CASE("Font initialization and error handling", "[resource][font]") {
    FT_Library lib = nullptr;
    REQUIRE(FT_Init_FreeType(&lib) == 0);
    RaiiGuard lib_guard([]() {}, [&lib]() { FT_Done_FreeType(lib); });

    SECTION("Non-existent font path throws std::runtime_error") {
        CHECK_THROWS_AS(Font("non_existent_font_file.ttf", 16, lib),
                        std::runtime_error);
    }
}

TEST_CASE("Font metrics and rasterization with real font", "[resource][font]") {
    fs::path font_path = get_test_font_path();
    REQUIRE_FALSE(font_path.empty());

    FT_Library lib = nullptr;
    REQUIRE(FT_Init_FreeType(&lib) == 0);
    RaiiGuard lib_guard([]() {}, [&lib]() { FT_Done_FreeType(lib); });

    constexpr size_t pixel_size = 16;
    Font font(font_path.string(), pixel_size, lib);

    SECTION("Metrics are properly initialized") {
        CHECK(font.pixel_size() == pixel_size);
        CHECK(font.ascender() > 0);
        CHECK(font.descender() <= 0);
        CHECK(font.line_height() > 0);
        CHECK(font.ft_face() != nullptr);
        CHECK(font.hb_font() != nullptr);
    }

    SECTION("Text shaping with ASCII and Unicode strings") {
        auto empty_glyphs = font.shape("");
        CHECK(empty_glyphs.empty());

        auto ascii_glyphs = font.shape("Hello");
        REQUIRE(ascii_glyphs.size() == 5);
        for (const auto& g : ascii_glyphs) {
            CHECK(g.glyph_id != 0);
            CHECK(g.x_advance > 0);
        }

        auto cjk_glyphs = font.shape("Serenkai 游戏");
        REQUIRE(cjk_glyphs.size() > 5);
        for (const auto& g : cjk_glyphs) {
            CHECK(g.x_advance > 0);
        }
    }

    SECTION("Glyph bitmap generation and caching") {
        auto glyphs = font.shape("A ");
        REQUIRE(glyphs.size() == 2);

        // Printable glyph 'A'
        const auto& glyph_a = font.get_glyph_bitmap(glyphs[0].glyph_id);
        CHECK(glyph_a.valid);
        CHECK(glyph_a.width > 0);
        CHECK(glyph_a.height > 0);
        CHECK(glyph_a.pixels.size() ==
              static_cast<size_t>(glyph_a.width * glyph_a.height));

        // Subsequent lookup must hit the cache and return the exact same
        // reference
        const auto& cached_a = font.get_glyph_bitmap(glyphs[0].glyph_id);
        CHECK(&glyph_a == &cached_a);

        // Space glyph has no visual pixels but is valid
        const auto& glyph_space = font.get_glyph_bitmap(glyphs[1].glyph_id);
        CHECK(glyph_space.valid);
        CHECK(glyph_space.width == 0);
        CHECK(glyph_space.height == 0);
        CHECK(glyph_space.pixels.empty());
    }
}
