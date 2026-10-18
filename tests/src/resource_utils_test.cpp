#include "serenkai/resource/resource_location.hpp"
#include "serenkai/resource/resource_utils.hpp"

#include <catch2/catch_test_macros.hpp>
#include <optional>

using namespace serenkai;

TEST_CASE("convert_aseprite_to_location error handling and edge cases",
          "[resource][resource_utils]") {
    const auto json_loc =
        *ResourceLocation::parse("serenkai:animations/hero.json");

    SECTION("Empty image file name returns nullopt") {
        auto result = convert_aseprite_to_location("", json_loc);
        CHECK_FALSE(result.has_value());
    }

    SECTION("Path escaping outside namespace root returns nullopt") {
        auto result =
            convert_aseprite_to_location("../../outside.png", json_loc);
        CHECK_FALSE(result.has_value());
    }

    SECTION("Malformed path with multiple colons returns nullopt") {
        auto result =
            convert_aseprite_to_location("bad:with:colons.png", json_loc);
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("convert_aseprite_to_location resolves image filenames",
          "[resource][resource_utils]") {
    const auto json_loc =
        *ResourceLocation::parse("serenkai:animations/hero.json");

    SECTION("Same-directory image filename") {
        auto result = convert_aseprite_to_location("hero.png", json_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:animations/hero.png");
    }

    SECTION("Subdirectory image path") {
        auto result =
            convert_aseprite_to_location("textures/hero.png", json_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:animations/textures/hero.png");
    }

    SECTION("Sibling directory image path") {
        auto result =
            convert_aseprite_to_location("../textures/hero.png", json_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:textures/hero.png");
    }

    SECTION("Preserves custom namespace") {
        const auto custom_loc =
            *ResourceLocation::parse("mod_ns:characters/hero.json");
        auto result = convert_aseprite_to_location("hero.png", custom_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "mod_ns:characters/hero.png");
    }
}

TEST_CASE("convert_tmx_to_location error handling and edge cases",
          "[resource][resource_utils]") {
    const auto map_loc = *ResourceLocation::parse("serenkai:maps/level1.tmx");
    const std::string map_path = "/project/assets/maps/level1.tmx";

    SECTION("Empty image path returns nullopt") {
        auto result = convert_tmx_to_location("", map_path, map_loc);
        CHECK_FALSE(result.has_value());
    }

    SECTION("Path navigating outside namespace root returns nullopt") {
        auto result = convert_tmx_to_location("/other/root/tileset.png",
                                              map_path, map_loc);
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("convert_tmx_to_location resolves disk paths",
          "[resource][resource_utils]") {
    const auto map_loc = *ResourceLocation::parse("serenkai:maps/level1.tmx");
    const std::string map_path = "/project/assets/maps/level1.tmx";

    SECTION("Absolute path in same directory as map") {
        auto result = convert_tmx_to_location(
            "/project/assets/maps/tileset.png", map_path, map_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:maps/tileset.png");
    }

    SECTION("Absolute path in sibling directory") {
        auto result = convert_tmx_to_location(
            "/project/assets/textures/terrain.png", map_path, map_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:textures/terrain.png");
    }

    SECTION("Relative map disk path with matching image path") {
        auto result = convert_tmx_to_location("maps/tileset.png",
                                              "maps/level1.tmx", map_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:maps/tileset.png");
    }

    SECTION("Preserves custom namespace") {
        const auto custom_map_loc =
            *ResourceLocation::parse("custom_ns:levels/dungeon.tmx");
        const std::string custom_map_path = "/game/assets/levels/dungeon.tmx";
        auto result = convert_tmx_to_location("/game/assets/levels/tiles.png",
                                              custom_map_path, custom_map_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "custom_ns:levels/tiles.png");
    }
}
