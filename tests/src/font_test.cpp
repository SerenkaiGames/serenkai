#include "serenkai/base/raii.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/font.hpp"
#include "serenkai/resource/font_manager.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <memory>
#include <stdexcept>
#include <utility>

namespace fs = std::filesystem;
using namespace serenkai;

namespace {

/// @brief Mock AssetSource for in-memory testing.
class MockAssetSource : public AssetSource {
public:
    explicit MockAssetSource(std::string name, AssetFileMap files = {})
        : m_name(std::move(name)), m_files(std::move(files)) {}

    AssetFileMap& get_asset_files() override { return m_files; }

    std::string source_name() const override { return m_name; }

private:
    std::string m_name;
    AssetFileMap m_files;
};

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

TEST_CASE("FontManager lifecycle, caching, and retrieval", "[resource][font]") {
    fs::path font_path = get_test_font_path();
    REQUIRE_FALSE(font_path.empty());

    AssetManager asset_manager;
    auto loc = *ResourceLocation::parse("serenkai:fonts/unifont.otf");
    AssetFileMap files;
    files.emplace(loc, font_path.string());
    asset_manager.merge_source(
        std::make_shared<MockAssetSource>("TestFontSource", std::move(files)));

    SECTION("Null AssetManager returns nullptr safely") {
        FontManager null_fm(nullptr);
        CHECK(null_fm.get("serenkai:fonts/unifont.otf") == nullptr);
    }

    SECTION("Valid font retrieval and default pixel size") {
        FontManager font_manager(&asset_manager);
        Font* font_default = font_manager.get("serenkai:fonts/unifont.otf");
        REQUIRE(font_default != nullptr);
        CHECK(font_default->pixel_size() == FontManager::DEFAULT_PIXEL_SIZE);

        // Subsequent lookup with the same font and size must return the exact
        // same pointer (cache hit)
        Font* cached_font = font_manager.get("serenkai:fonts/unifont.otf");
        CHECK(font_default == cached_font);
    }

    SECTION("Different pixel sizes are cached independently") {
        FontManager font_manager(&asset_manager);
        Font* font_16 = font_manager.get("serenkai:fonts/unifont.otf", 16);
        REQUIRE(font_16 != nullptr);
        CHECK(font_16->pixel_size() == 16);

        Font* font_24 = font_manager.get("serenkai:fonts/unifont.otf", 24);
        REQUIRE(font_24 != nullptr);
        CHECK(font_24->pixel_size() == 24);

        CHECK(font_16 != font_24);

        Font* font_16_cached =
            font_manager.get("serenkai:fonts/unifont.otf", 16);
        CHECK(font_16 == font_16_cached);
    }

    SECTION("Non-existent asset or invalid location returns nullptr") {
        FontManager font_manager(&asset_manager);
        CHECK(font_manager.get("serenkai:fonts/does_not_exist.otf") == nullptr);
        CHECK(font_manager.get("invalid$location") == nullptr);
        CHECK(font_manager.get("") == nullptr);
    }
}
