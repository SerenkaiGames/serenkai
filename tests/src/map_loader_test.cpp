#include "serenkai/base/raii.hpp"
#include "serenkai/game/map_data.hpp"
#include "serenkai/resource/asset_manager.hpp"
#include "serenkai/resource/asset_source.hpp"
#include "serenkai/resource/map_loader.hpp"
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

TEST_CASE("Tile and FlipFlag operations", "[game][map_data]") {
    SECTION("Default constructed Tile is empty") {
        Tile tile{};
        CHECK(tile.is_empty());
        CHECK(tile.gid == 0);
        CHECK(tile.flag == Tile::FlipFlag::None);
        CHECK(tile.tileset == 0);
        CHECK(tile.local_id == 0);
        CHECK_FALSE(tile.flip_horizontal());
        CHECK_FALSE(tile.flip_vertical());
        CHECK_FALSE(tile.flip_diagonal());
    }

    SECTION("FlipFlag bitwise operators and queries") {
        Tile tile{};
        tile.gid = 12;
        CHECK_FALSE(tile.is_empty());

        tile.flag |= Tile::FlipFlag::Horizontal;
        CHECK(tile.flip_horizontal());
        CHECK_FALSE(tile.flip_vertical());
        CHECK_FALSE(tile.flip_diagonal());

        tile.flag |= Tile::FlipFlag::Vertical;
        CHECK(tile.flip_horizontal());
        CHECK(tile.flip_vertical());
        CHECK_FALSE(tile.flip_diagonal());

        tile.flag |= Tile::FlipFlag::Diagonal;
        CHECK(tile.flip_horizontal());
        CHECK(tile.flip_vertical());
        CHECK(tile.flip_diagonal());

        CHECK((tile.flag & Tile::FlipFlag::Horizontal) ==
              Tile::FlipFlag::Horizontal);
        CHECK((tile.flag & Tile::FlipFlag::Vertical) ==
              Tile::FlipFlag::Vertical);
        CHECK((tile.flag & Tile::FlipFlag::Diagonal) ==
              Tile::FlipFlag::Diagonal);
    }
}

TEST_CASE("TileLayer 2D coordinate access and bounds checking",
          "[game][map_data]") {
    TileLayer layer{};
    layer.name = "Background";
    layer.size = {3, 2};

    for (std::uint32_t i = 1; i <= 6; ++i) {
        Tile t{};
        t.gid = i;
        layer.tiles.push_back(t);
    }

    REQUIRE(layer.tiles.size() == 6);

    SECTION("In-bounds coordinate access") {
        const auto* t00 = layer.get_tile(0, 0);
        REQUIRE(t00 != nullptr);
        CHECK(t00->gid == 1);

        const auto* t20 = layer.get_tile(2, 0);
        REQUIRE(t20 != nullptr);
        CHECK(t20->gid == 3);

        const auto* t01 = layer.get_tile(0, 1);
        REQUIRE(t01 != nullptr);
        CHECK(t01->gid == 4);

        const auto* t21 = layer.get_tile(2, 1);
        REQUIRE(t21 != nullptr);
        CHECK(t21->gid == 6);

        const auto* t11 = layer.get_tile(glm::ivec2{1, 1});
        REQUIRE(t11 != nullptr);
        CHECK(t11->gid == 5);
    }

    SECTION("Out-of-bounds coordinate access returns nullptr") {
        CHECK(layer.get_tile(-1, 0) == nullptr);
        CHECK(layer.get_tile(3, 0) == nullptr);
        CHECK(layer.get_tile(0, -1) == nullptr);
        CHECK(layer.get_tile(0, 2) == nullptr);
        CHECK(layer.get_tile(glm::ivec2{5, 5}) == nullptr);
    }
}

TEST_CASE("Tileset get_rect calculations and safety", "[game][map_data]") {
    Tileset ts{};
    ts.name = "Terrain";
    ts.tile_size = {16, 16};
    ts.margin = 2;
    ts.spacing = 1;
    ts.column_count = 4;

    SECTION("Valid rect calculation") {
        auto r0 = ts.get_rect(0);
        CHECK(r0.x == 2.0f);
        CHECK(r0.y == 2.0f);
        CHECK(r0.z == 16.0f);
        CHECK(r0.w == 16.0f);

        // local_id = 5 => column 1, row 1
        auto r5 = ts.get_rect(5);
        CHECK(r5.x == 2.0f + 1.0f * (16.0f + 1.0f));
        CHECK(r5.y == 2.0f + 1.0f * (16.0f + 1.0f));
        CHECK(r5.z == 16.0f);
        CHECK(r5.w == 16.0f);
    }

    SECTION("Zero column_count does not divide by zero") {
        ts.column_count = 0;
        auto r_safe = ts.get_rect(5);
        CHECK(r_safe == glm::vec4(0.0f));
    }
}

TEST_CASE("MapProperty variant value storage and access", "[game][map_data]") {
    SECTION("Boolean property") {
        MapProperty prop{"collidable", true};
        CHECK(prop.is<bool>());
        CHECK_FALSE(prop.is<int>());
        CHECK(prop.get_or<bool>(false) == true);
        CHECK(prop.get_or<int>(99) == 99);
    }

    SECTION("Integer property") {
        MapProperty prop{"damage", 42};
        CHECK(prop.is<int>());
        CHECK(prop.get_or<int>(0) == 42);
        REQUIRE(prop.get_if<int>() != nullptr);
        CHECK(*prop.get_if<int>() == 42);
    }

    SECTION("Float property") {
        MapProperty prop{"friction", 0.75f};
        CHECK(prop.is<float>());
        CHECK(prop.get_or<float>(0.0f) == 0.75f);
    }

    SECTION("String property") {
        MapProperty prop{"script", std::string("player.lua")};
        CHECK(prop.is<std::string>());
        CHECK(prop.get_or<std::string>("") == "player.lua");
    }

    SECTION("Color property (vec4)") {
        glm::vec4 color{1.0f, 0.5f, 0.25f, 1.0f};
        MapProperty prop{"tint", color};
        CHECK(prop.is<glm::vec4>());
        CHECK(prop.get_or<glm::vec4>(glm::vec4{0.0f}) == color);
    }
}

TEST_CASE("MapLoader error handling and edge cases", "[resource][map_loader]") {
    AssetManager asset_manager;
    MapLoader loader(&asset_manager);

    SECTION("Non-existent asset returns nullopt") {
        auto result = loader.load("serenkai:maps/non_existent.tmx");
        CHECK_FALSE(result.has_value());
    }

    SECTION("Invalid resource location returns nullopt") {
        auto result = loader.load(":invalid_loc");
        CHECK_FALSE(result.has_value());
    }

    SECTION("Corrupted XML file returns nullopt") {
        fs::path temp_dir =
            fs::temp_directory_path() / "serenkai_test_map_corrupted";
        fs::create_directories(temp_dir);
        RaiiGuard cleanup_guard([]() {},
                                [&temp_dir]() {
                                    std::error_code ec;
                                    fs::remove_all(temp_dir, ec);
                                });

        fs::path map_path = temp_dir / "corrupted.tmx";
        {
            std::ofstream out(map_path);
            out << "not an xml content <map>";
        }

        auto loc = *ResourceLocation::parse("test:corrupted.tmx");
        AssetFileMap files;
        files.emplace(loc, map_path.string());
        asset_manager.merge_source(std::make_shared<MockMapAssetSource>(
            "MockCorrupt", std::move(files)));

        auto result = loader.load("test:corrupted.tmx");
        CHECK_FALSE(result.has_value());
    }

    SECTION("Non-orthogonal map returns nullopt") {
        fs::path temp_dir =
            fs::temp_directory_path() / "serenkai_test_map_isometric";
        fs::create_directories(temp_dir);
        RaiiGuard cleanup_guard([]() {},
                                [&temp_dir]() {
                                    std::error_code ec;
                                    fs::remove_all(temp_dir, ec);
                                });

        fs::path map_path = temp_dir / "isometric.tmx";
        {
            std::ofstream out(map_path);
            out << R"(<?xml version="1.0" encoding="UTF-8"?>
<map version="1.10" tiledversion="1.10.2" orientation="isometric" renderorder="right-down" width="2" height="2" tilewidth="32" tileheight="16" infinite="0">
</map>)";
        }

        auto loc = *ResourceLocation::parse("test:isometric.tmx");
        AssetFileMap files;
        files.emplace(loc, map_path.string());
        asset_manager.merge_source(
            std::make_shared<MockMapAssetSource>("MockIso", std::move(files)));

        auto result = loader.load("test:isometric.tmx");
        CHECK_FALSE(result.has_value());
    }

    SECTION("Infinite map returns nullopt") {
        fs::path temp_dir =
            fs::temp_directory_path() / "serenkai_test_map_infinite";
        fs::create_directories(temp_dir);
        RaiiGuard cleanup_guard([]() {},
                                [&temp_dir]() {
                                    std::error_code ec;
                                    fs::remove_all(temp_dir, ec);
                                });

        fs::path map_path = temp_dir / "infinite.tmx";
        {
            std::ofstream out(map_path);
            out << R"(<?xml version="1.0" encoding="UTF-8"?>
<map version="1.10" tiledversion="1.10.2" orientation="orthogonal" renderorder="right-down" width="2" height="2" tilewidth="16" tileheight="16" infinite="1">
</map>)";
        }

        auto loc = *ResourceLocation::parse("test:infinite.tmx");
        AssetFileMap files;
        files.emplace(loc, map_path.string());
        asset_manager.merge_source(
            std::make_shared<MockMapAssetSource>("MockInf", std::move(files)));

        auto result = loader.load("test:infinite.tmx");
        CHECK_FALSE(result.has_value());
    }
}

TEST_CASE("MapLoader successfully loads valid orthogonal TMX map",
          "[resource][map_loader]") {
    fs::path temp_dir =
        fs::temp_directory_path() / "serenkai_test_map_valid_load";
    fs::create_directories(temp_dir);
    RaiiGuard cleanup_guard([]() {},
                            [&temp_dir]() {
                                std::error_code ec;
                                fs::remove_all(temp_dir, ec);
                            });

    fs::path map_path = temp_dir / "valid_sample.tmx";
    {
        std::ofstream out(map_path);
        // Map with 2x2 grid, 16x16 tiles, red background (#ff0000)
        // Tileset with firstgid=1, 2 columns, 4 tiles
        // Layer 1: Ground with tiles: (gid 1), (gid 0 empty), (gid 2), (gid 3
        // with Horizontal flip 0x80000000 -> 2147483651) Layer 2: Object group
        // with a rectangle object and properties
        out << R"(<?xml version="1.0" encoding="UTF-8"?>
<map version="1.10" tiledversion="1.10.2" orientation="orthogonal" renderorder="right-down" width="2" height="2" tilewidth="16" tileheight="16" infinite="0" backgroundcolor="#ff0000">
 <tileset firstgid="1" name="sample_tileset" tilewidth="16" tileheight="16" tilecount="4" columns="2">
  <image source="sample.png" width="32" height="32"/>
 </tileset>
 <layer id="1" name="Ground" width="2" height="2" visible="1">
  <properties>
   <property name="collidable" type="bool" value="true"/>
   <property name="z_index" type="int" value="5"/>
   <property name="friction" type="float" value="0.75"/>
   <property name="tag" value="ground_layer"/>
  </properties>
  <data encoding="csv">
1,0,
2,2147483651
</data>
 </layer>
 <objectgroup id="2" name="Entities" visible="1">
  <object id="1" name="SpawnPoint" type="PlayerSpawn" x="8" y="16" width="16" height="16" rotation="45">
   <properties>
    <property name="active" type="bool" value="true"/>
    <property name="id" type="int" value="42"/>
   </properties>
  </object>
 </objectgroup>
</map>)";
    }

    AssetManager asset_manager;
    auto loc = *ResourceLocation::parse("test:maps/valid_sample.tmx");
    AssetFileMap files;
    files.emplace(loc, map_path.string());
    asset_manager.merge_source(
        std::make_shared<MockMapAssetSource>("MockValid", std::move(files)));

    MapLoader loader(&asset_manager);
    auto map_data = loader.load("test:maps/valid_sample.tmx");
    REQUIRE(map_data.has_value());

    CHECK(map_data->map_size == glm::ivec2{2, 2});
    CHECK(map_data->tile_size == glm::ivec2{16, 16});
    CHECK(map_data->background_color.r == 1.0f);
    CHECK(map_data->background_color.g == 0.0f);
    CHECK(map_data->background_color.b == 0.0f);

    // Verify tileset
    REQUIRE(map_data->tilesets.size() == 1);
    const auto& ts = map_data->tilesets[0];
    CHECK(ts.name == "sample_tileset");
    CHECK(ts.tile_size == glm::ivec2{16, 16});
    CHECK(ts.column_count == 2);
    CHECK(ts.tile_count == 4);

    // Verify layers
    REQUIRE(map_data->layers.size() == 2);

    // Layer 0: TileLayer
    REQUIRE(std::holds_alternative<TileLayer>(map_data->layers[0]));
    const auto& tile_layer = std::get<TileLayer>(map_data->layers[0]);
    CHECK(tile_layer.name == "Ground");
    CHECK(tile_layer.visible);
    CHECK(tile_layer.size == glm::ivec2{2, 2});
    REQUIRE(tile_layer.tiles.size() == 4);

    // Tile 0: gid 1
    CHECK(tile_layer.tiles[0].gid == 1);
    CHECK_FALSE(tile_layer.tiles[0].is_empty());
    CHECK(tile_layer.tiles[0].tileset == 0);
    CHECK(tile_layer.tiles[0].local_id == 0);

    // Tile 1: gid 0 (empty)
    CHECK(tile_layer.tiles[1].gid == 0);
    CHECK(tile_layer.tiles[1].is_empty());

    // Tile 2: gid 2
    CHECK(tile_layer.tiles[2].gid == 2);
    CHECK_FALSE(tile_layer.tiles[2].is_empty());
    CHECK(tile_layer.tiles[2].tileset == 0);
    CHECK(tile_layer.tiles[2].local_id == 1);

    // Tile 3: gid 3 with horizontal flip
    CHECK(tile_layer.tiles[3].gid == 3);
    CHECK(tile_layer.tiles[3].flip_horizontal());

    // Layer properties
    REQUIRE(tile_layer.properties.size() == 4);
    for (const auto& prop : tile_layer.properties) {
        if (prop.name == "collidable") {
            CHECK(prop.get_or<bool>(false) == true);
        } else if (prop.name == "z_index") {
            CHECK(prop.get_or<int>(0) == 5);
        } else if (prop.name == "friction") {
            CHECK(prop.get_or<float>(0.0f) == 0.75f);
        } else if (prop.name == "tag") {
            CHECK(prop.get_or<std::string>("") == "ground_layer");
        }
    }

    // Layer 1: ObjectGroup
    REQUIRE(std::holds_alternative<ObjectGroup>(map_data->layers[1]));
    const auto& obj_group = std::get<ObjectGroup>(map_data->layers[1]);
    CHECK(obj_group.name == "Entities");
    CHECK(obj_group.visible);
    REQUIRE(obj_group.objects.size() == 1);

    const auto& obj = obj_group.objects[0];
    CHECK(obj.name == "SpawnPoint");
    CHECK(obj.type == "PlayerSpawn");
    CHECK(obj.visible);
    CHECK(obj.rotation == 45.0f);
    CHECK(obj.aabb.x == 8.0f);
    CHECK(obj.aabb.y == 16.0f);
    CHECK(obj.aabb.z == 16.0f);
    CHECK(obj.aabb.w == 16.0f);

    REQUIRE(obj.properties.size() == 2);
    for (const auto& prop : obj.properties) {
        if (prop.name == "active") {
            CHECK(prop.get_or<bool>(false) == true);
        } else if (prop.name == "id") {
            CHECK(prop.get_or<int>(0) == 42);
        }
    }
}
