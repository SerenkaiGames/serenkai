#include "serenkai/resource/font.hpp"

#include <catch2/catch_test_macros.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H
#include <stdexcept>

using namespace serenkai;

TEST_CASE("Font initialization and error handling", "[resource][font]") {
    FT_Library lib = nullptr;
    REQUIRE(FT_Init_FreeType(&lib) == 0);

    SECTION("Non-existent font path throws std::runtime_error") {
        CHECK_THROWS_AS(Font("non_existent_font_file.ttf", 16, lib),
                        std::runtime_error);
    }

    FT_Done_FreeType(lib);
}
