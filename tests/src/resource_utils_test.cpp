#include "serenkai/resource/resource_location.hpp"
#include "serenkai/resource/resource_utils.hpp"

#include <catch2/catch_test_macros.hpp>
#include <optional>

namespace fs = std::filesystem;
using namespace serenkai;

TEST_CASE("resolve_relative_resource_location handles empty and invalid inputs",
          "[resource][resource_utils]") {
    const auto parent_loc =
        *ResourceLocation::parse("serenkai:animations/player.json");
    const std::string parent_disk_path =
        "/project/assets/animations/player.json";

    SECTION("Empty target path returns nullopt") {
        auto result = resolve_relative_resource_location("", parent_disk_path,
                                                         parent_loc);
        CHECK_FALSE(result.has_value());
    }

    SECTION("Target path navigating outside namespace returns nullopt") {
        auto result = resolve_relative_resource_location(
            "../../outside.png", parent_disk_path, parent_loc);
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("resolve_relative_resource_location resolves relative paths",
          "[resource][resource_utils]") {
    const auto parent_loc =
        *ResourceLocation::parse("serenkai:animations/hero.json");
    const std::string parent_disk_path = "/project/assets/animations/hero.json";

    SECTION("Same-directory relative file path") {
        auto result = resolve_relative_resource_location(
            "hero.png", parent_disk_path, parent_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:animations/hero.png");
    }

    SECTION("Relative path with explicit dot-slash") {
        auto result = resolve_relative_resource_location(
            "./hero.png", parent_disk_path, parent_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:animations/hero.png");
    }

    SECTION("Relative path in subdirectory") {
        auto result = resolve_relative_resource_location(
            "textures/hero.png", parent_disk_path, parent_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:animations/textures/hero.png");
    }

    SECTION("Relative path traversing up to sibling directory") {
        auto result = resolve_relative_resource_location(
            "../textures/hero.png", parent_disk_path, parent_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:textures/hero.png");
    }

    SECTION("Preserves custom namespace") {
        const auto custom_parent_loc =
            *ResourceLocation::parse("mod_ns:characters/hero.json");
        auto result = resolve_relative_resource_location(
            "hero.png", parent_disk_path, custom_parent_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "mod_ns:characters/hero.png");
    }
}

TEST_CASE("resolve_relative_resource_location resolves absolute disk paths",
          "[resource][resource_utils]") {
    const auto parent_loc =
        *ResourceLocation::parse("serenkai:maps/level1.tmx");
    const std::string parent_disk_path = "/project/assets/maps/level1.tmx";

    SECTION("Absolute path in same directory") {
        auto result = resolve_relative_resource_location(
            "/project/assets/maps/tileset.png", parent_disk_path, parent_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:maps/tileset.png");
    }

    SECTION("Absolute path in sibling directory") {
        auto result = resolve_relative_resource_location(
            "/project/assets/textures/terrain.png", parent_disk_path,
            parent_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:textures/terrain.png");
    }

    SECTION("Disk path prefixed by relative parent directory") {
        auto result = resolve_relative_resource_location(
            "maps/tileset.png", "maps/level1.tmx", parent_loc);
        REQUIRE(result.has_value());
        CHECK(result->str() == "serenkai:maps/tileset.png");
    }
}
