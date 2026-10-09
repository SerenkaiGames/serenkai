#include "serenkai/base/raii.hpp"
#include "serenkai/game/map_manager.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <utility>

namespace fs = std::filesystem;
using namespace serenkai;

namespace {

/// @brief Mock AssetSource for registering map file paths in AssetManager.
class MockMapAssetSource : public AssetSource {
public:
    explicit MockMapAssetSource(std::string name, AssetFileMap files = {})
        : m_name(std::move(name)), m_files(std::move(files)) {}

    AssetFileMap& get_asset_files() override { return m_files; }
    std::string source_name() const override { return m_name; }

private:
    std::string m_name;
    AssetFileMap m_files;
};

} // namespace

TEST_CASE("MapManager initial state and null safety", "[game][map_manager]") {
    AssetManager asset_manager;
    MapManager manager(&asset_manager);

    SECTION("Initial map query returns empty") {
        CHECK_FALSE(manager.has_map());
        CHECK(manager.current_map() == nullptr);
    }

    SECTION("Update without map is safe") {
        // Calling update when no map is loaded should not throw or crash
        manager.update(0.016f);
        CHECK_FALSE(manager.has_map());
    }
}

TEST_CASE("MapManager error handling on invalid map location",
          "[game][map_manager]") {
    AssetManager asset_manager;
    MapManager manager(&asset_manager);

    SECTION("Empty location string") {
        CHECK_FALSE(manager.switch_map(""));
        CHECK_FALSE(manager.has_map());
        CHECK(manager.current_map() == nullptr);
    }

    SECTION("Malformed ResourceLocation string") {
        CHECK_FALSE(manager.switch_map(":invalid_location"));
        CHECK_FALSE(manager.has_map());
        CHECK(manager.current_map() == nullptr);
    }

    SECTION("Unregistered resource location") {
        CHECK_FALSE(manager.switch_map("core:maps/non_existent.tmx"));
        CHECK_FALSE(manager.has_map());
        CHECK(manager.current_map() == nullptr);
    }
}

TEST_CASE("MapManager load, switch, and caching", "[game][map_manager]") {
    fs::path temp_dir =
        fs::temp_directory_path() / "serenkai_test_map_manager_cache";
    fs::create_directories(temp_dir);
    RaiiGuard cleanup_guard([]() {},
                            [&temp_dir]() {
                                std::error_code ec;
                                fs::remove_all(temp_dir, ec);
                            });

    fs::path map_path = temp_dir / "sample.tmx";
    {
        std::ofstream out(map_path);
        out << R"(<?xml version="1.0" encoding="UTF-8"?>
<map version="1.10" tiledversion="1.10.2" orientation="orthogonal" renderorder="right-down" width="2" height="2" tilewidth="16" tileheight="16" infinite="0">
 <layer id="1" name="Ground" width="2" height="2">
  <data encoding="csv">
1,2,
0,0
</data>
 </layer>
</map>)";
    }

    AssetManager asset_manager;
    auto loc = *ResourceLocation::parse("core:maps/sample.tmx");
    AssetFileMap files;
    files.emplace(loc, map_path.string());
    asset_manager.merge_source(std::make_shared<MockMapAssetSource>(
        "MockMapSource", std::move(files)));

    MapManager manager(&asset_manager);

    SECTION("Successful map load and switch") {
        REQUIRE(manager.switch_map("core:maps/sample.tmx"));
        CHECK(manager.has_map());

        auto current = manager.current_map();
        REQUIRE(current != nullptr);
        CHECK(current->map_size() == glm::ivec2(2, 2));
        CHECK(current->tile_size() == glm::ivec2(16, 16));
        CHECK(current->has_layer("Ground"));
    }

    SECTION("Map cache hit returns identical shared instance") {
        REQUIRE(manager.switch_map("core:maps/sample.tmx"));
        auto first_instance = manager.current_map();
        REQUIRE(first_instance != nullptr);

        REQUIRE(manager.switch_map("core:maps/sample.tmx"));
        auto second_instance = manager.current_map();
        REQUIRE(second_instance != nullptr);

        CHECK(first_instance.get() == second_instance.get());
    }

    SECTION("Update propagates to loaded map") {
        REQUIRE(manager.switch_map("core:maps/sample.tmx"));
        auto current = manager.current_map();
        REQUIRE(current != nullptr);

        current->mark_object_dirty(true);
        CHECK(current->is_object_dirty());

        manager.update(0.016f);
        CHECK_FALSE(current->is_object_dirty());
    }
}
