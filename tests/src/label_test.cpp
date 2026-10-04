#include "serenkai/base/raii.hpp"
#include "serenkai/gui/label.hpp"
#include "serenkai/resource/font.hpp"

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

TEST_CASE("Label default state and property setters", "[gui][label]") {
    Label label("label", nullptr);

    CHECK(label.text().empty());
    CHECK(label.color() == Color::White);
    CHECK(label.font() == nullptr);
    CHECK(label.size() == glm::ivec2{0, 0});

    SECTION("Text mutation") {
        label.set_text("Hello");
        CHECK(label.text() == "Hello");
    }

    SECTION("Color mutation") {
        label.set_color(Color::Red);
        CHECK(label.color() == Color::Red);
    }

    SECTION("Nullptr font safety") {
        label.set_font(nullptr);
        CHECK(label.font() == nullptr);
        CHECK(label.size() == glm::ivec2{0, 0});
    }
}

TEST_CASE("Label size measurement and rendering with font", "[gui][label]") {
    fs::path font_path = get_test_font_path();
    REQUIRE_FALSE(font_path.empty());

    FT_Library lib = nullptr;
    REQUIRE(FT_Init_FreeType(&lib) == 0);
    RaiiGuard lib_guard([]() {}, [&lib]() { FT_Done_FreeType(lib); });

    constexpr size_t pixel_size = 16;
    Font font(font_path.string(), pixel_size, lib);

    Label label("label", nullptr);

    SECTION("Setting text before font computes size once font is set") {
        label.set_text("Serenkai");
        CHECK(label.size() == glm::ivec2{0, 0});

        label.set_font(&font);
        CHECK(label.font() == &font);
        CHECK(label.size().x == font.measure_width("Serenkai"));
        CHECK(label.size().y == font.line_height());
        CHECK(label.size().x > 0);
        CHECK(label.size().y > 0);
    }

    SECTION("Updating text with font bound updates size") {
        label.set_font(&font);
        label.set_text("A");
        int width_a = label.size().x;
        CHECK(width_a > 0);

        label.set_text("AAAA");
        int width_aaaa = label.size().x;
        CHECK(width_aaaa > width_a);
        CHECK(label.size().y == font.line_height());
    }

    SECTION("Render safety without context or font") {
        // Without font
        label.render(nullptr);

        // With font but without context
        label.set_font(&font);
        label.set_text("Test");
        label.render(nullptr);
    }
}
