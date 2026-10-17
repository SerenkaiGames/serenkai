#include "serenkai/base/string_hash.hpp"
#include "serenkai/game/map.hpp"
#include "serenkai/game/map_data.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>
#include <string_view>
#include <utility>

using namespace serenkai;

TEST_CASE("Map dimensions and coordinate conversions", "[game][map]") {
    MapData data{};
    data.map_size = {10, 8};
    data.tile_size = {16, 16};

    Map map(std::move(data));

    SECTION("Dimension queries") {
        CHECK(map.map_size() == glm::ivec2(10, 8));
        CHECK(map.tile_size() == glm::ivec2(16, 16));
        CHECK(map.pixel_size() == glm::ivec2(160, 128));
    }

    SECTION("Bounds checking") {
        CHECK(map.is_in_bounds({0, 0}));
        CHECK(map.is_in_bounds({9, 7}));
        CHECK(map.is_in_bounds({5, 4}));

        CHECK_FALSE(map.is_in_bounds({-1, 0}));
        CHECK_FALSE(map.is_in_bounds({0, -1}));
        CHECK_FALSE(map.is_in_bounds({10, 0}));
        CHECK_FALSE(map.is_in_bounds({0, 8}));
        CHECK_FALSE(map.is_in_bounds({10, 8}));
    }

    SECTION("Coordinate conversion round-trip and flooring") {
        CHECK(map.world_to_tile({0.0f, 0.0f}) == glm::ivec2(0, 0));
        CHECK(map.world_to_tile({15.9f, 15.9f}) == glm::ivec2(0, 0));
        CHECK(map.world_to_tile({16.0f, 32.0f}) == glm::ivec2(1, 2));
        CHECK(map.world_to_tile({31.9f, 47.9f}) == glm::ivec2(1, 2));

        CHECK(map.tile_to_world({0, 0}) == glm::vec2(0.0f, 0.0f));
        CHECK(map.tile_to_world({1, 2}) == glm::vec2(16.0f, 32.0f));
        CHECK(map.tile_to_world({9, 7}) == glm::vec2(144.0f, 112.0f));
    }
}

TEST_CASE("Map layer queries and tile access", "[game][map]") {
    MapData data{};
    data.map_size = {3, 2};
    data.tile_size = {16, 16};

    TileLayer ground{};
    ground.name = "Ground";
    ground.size = {3, 2};
    ground.visible = true;

    for (std::uint32_t i = 1; i <= 6; ++i) {
        Tile t{};
        t.gid = i;
        ground.tiles.push_back(t);
    }
    data.layers.emplace_back(std::move(ground));

    Map map(std::move(data));

    SECTION("Layer existence queries") {
        CHECK(map.has_layer("Ground"));
        CHECK_FALSE(map.has_layer("Sky"));
        CHECK_FALSE(map.has_layer(""));
    }

    SECTION("Layer visibility modification") {
        map.set_layer_visible("Ground", false);
        const auto& layer = std::get<TileLayer>(map.data().layers[0]);
        CHECK_FALSE(layer.visible);

        map.set_layer_visible("Ground", true);
        CHECK(std::get<TileLayer>(map.data().layers[0]).visible);

        // Modifying non-existent layer does nothing
        map.set_layer_visible("Sky", false);
    }

    SECTION("Tile retrieval with bounds and layer validation") {
        auto t00 = map.get_tile("Ground", {0, 0});
        REQUIRE(t00.has_value());
        CHECK(t00->gid == 1);

        auto t20 = map.get_tile("Ground", {2, 0});
        REQUIRE(t20.has_value());
        CHECK(t20->gid == 3);

        auto t21 = map.get_tile("Ground", {2, 1});
        REQUIRE(t21.has_value());
        CHECK(t21->gid == 6);

        // Out-of-bounds queries
        CHECK_FALSE(map.get_tile("Ground", {-1, 0}).has_value());
        CHECK_FALSE(map.get_tile("Ground", {3, 0}).has_value());
        CHECK_FALSE(map.get_tile("Ground", {0, 2}).has_value());

        // Non-existent layer
        CHECK_FALSE(map.get_tile("NonExistent", {0, 0}).has_value());
    }
}

TEST_CASE("Map object lookups and index querying", "[game][map]") {
    MapData data{};
    data.map_size = {10, 10};
    data.tile_size = {16, 16};

    ObjectGroup group{};
    group.name = "Entities";

    MapObject p_spawn{};
    p_spawn.name = "player_spawn";
    p_spawn.type = "spawn";
    p_spawn.pos = {16.0f, 32.0f};

    MapObject e_spawn{};
    e_spawn.name = "enemy_spawn";
    e_spawn.type = "spawn";
    e_spawn.pos = {64.0f, 64.0f};

    MapObject npc{};
    npc.name = "guide_npc";
    npc.type = "npc";
    npc.pos = {128.0f, 128.0f};

    MapObject alt_spawn{};
    alt_spawn.name = "player_spawn";
    alt_spawn.type = "checkpoint";
    alt_spawn.pos = {200.0f, 200.0f};

    group.objects = {p_spawn, e_spawn, npc, alt_spawn};
    data.layers.emplace_back(std::move(group));

    Map map(std::move(data));

    SECTION("Find object by name") {
        auto found_spawns = map.find_object("player_spawn");
        REQUIRE(found_spawns.size() == 2);
        CHECK(found_spawns[0].name == "player_spawn");
        CHECK(found_spawns[0].pos.x == 16.0f);
        CHECK(found_spawns[1].name == "player_spawn");
        CHECK(found_spawns[1].pos.x == 200.0f);

        auto found_npc = map.find_object("guide_npc");
        REQUIRE(found_npc.size() == 1);
        CHECK(found_npc[0].type == "npc");

        CHECK(map.find_object("non_existent").empty());
    }

    SECTION("Find objects by type") {
        auto spawns = map.find_objects_by_type("spawn");
        REQUIRE(spawns.size() == 2);
        CHECK(spawns[0].name == "player_spawn");
        CHECK(spawns[1].name == "enemy_spawn");

        auto npcs = map.find_objects_by_type("npc");
        REQUIRE(npcs.size() == 1);
        CHECK(npcs[0].name == "guide_npc");

        auto checkpoints = map.find_objects_by_type("checkpoint");
        REQUIRE(checkpoints.size() == 1);
        CHECK(checkpoints[0].name == "player_spawn");

        CHECK(map.find_objects_by_type("non_existent").empty());
    }
}

TEST_CASE("Map spawn point queries", "[game][map]") {
    MapData data{};
    data.map_size = {10, 10};
    data.tile_size = {16, 16};

    ObjectGroup group{};
    group.name = "Entities";

    MapObject single_spawn{};
    single_spawn.name = "player_spawn";
    single_spawn.aabb = {10.0f, 20.0f, 30.0f, 40.0f};

    MapObject point_spawn{};
    point_spawn.name = "point_spawn";
    point_spawn.aabb = {15.0f, 25.0f, 0.0f, 0.0f};

    MapObject dup_spawn_1{};
    dup_spawn_1.name = "dup_spawn";
    dup_spawn_1.aabb = {0.0f, 0.0f, 10.0f, 20.0f};

    MapObject dup_spawn_2{};
    dup_spawn_2.name = "dup_spawn";
    dup_spawn_2.aabb = {100.0f, 100.0f, 20.0f, 20.0f};

    group.objects = {single_spawn, point_spawn, dup_spawn_1, dup_spawn_2};
    data.layers.emplace_back(std::move(group));

    Map map(std::move(data));

    SECTION("Retrieves center of bounding box for single spawn point") {
        auto spawn = map.get_spawn_point("player_spawn");
        REQUIRE(spawn.has_value());
        CHECK(spawn->x == 25.0f);
        CHECK(spawn->y == 40.0f);
    }

    SECTION("Retrieves position for zero-size point spawn") {
        auto spawn = map.get_spawn_point("point_spawn");
        REQUIRE(spawn.has_value());
        CHECK(spawn->x == 15.0f);
        CHECK(spawn->y == 25.0f);
    }

    SECTION("Returns first matching object when duplicates exist") {
        auto spawn = map.get_spawn_point("dup_spawn");
        REQUIRE(spawn.has_value());
        CHECK(spawn->x == 5.0f);
        CHECK(spawn->y == 10.0f);
    }

    SECTION("Returns std::nullopt for non-existent spawn point") {
        auto spawn = map.get_spawn_point("non_existent");
        CHECK_FALSE(spawn.has_value());
    }
}

TEST_CASE("Map dirty flags and update lifecycle", "[game][map]") {
    MapData data{};
    Map map(std::move(data));

    SECTION("Dirty flag toggles") {
        CHECK_FALSE(map.is_tile_dirty());
        CHECK_FALSE(map.is_object_dirty());

        map.mark_tile_dirty(true);
        CHECK(map.is_tile_dirty());

        map.mark_tile_dirty(false);
        CHECK_FALSE(map.is_tile_dirty());

        map.mark_object_dirty(true);
        CHECK(map.is_object_dirty());

        map.mark_object_dirty(false);
        CHECK_FALSE(map.is_object_dirty());
    }

    SECTION("Update resets object dirty flag") {
        map.mark_object_dirty(true);
        CHECK(map.is_object_dirty());

        map.update(0.016f);
        CHECK_FALSE(map.is_object_dirty());
    }
}

TEST_CASE("Map collision detection and solid queries",
          "[game][map][collision]") {
    MapData data{};
    data.map_size = {3, 3};
    data.tile_size = {16, 16};

    TileLayer col_layer{};
    col_layer.name = "collision";
    col_layer.size = {3, 3};
    col_layer.visible = true;

    // Grid layout (x from 0 to 2, y from 0 to 2):
    // Row 0: empty(0), empty(0), solid(1)
    // Row 1: empty(0), solid(2), empty(0)
    // Row 2: empty(0), empty(0), empty(0)
    const std::vector<std::uint32_t> gids = {0, 0, 1, 0, 2, 0, 0, 0, 0};
    for (std::uint32_t gid : gids) {
        Tile t{};
        t.gid = gid;
        col_layer.tiles.push_back(t);
    }
    data.layers.emplace_back(std::move(col_layer));
    Map map(std::move(data));

    SECTION("is_solid checks for in-bounds and out-of-bounds tiles") {
        // In-bounds empty tiles
        CHECK_FALSE(map.is_solid({0, 0}));
        CHECK_FALSE(map.is_solid({1, 0}));
        CHECK_FALSE(map.is_solid({0, 1}));
        CHECK_FALSE(map.is_solid({2, 1}));
        CHECK_FALSE(map.is_solid({0, 2}));
        CHECK_FALSE(map.is_solid({1, 2}));
        CHECK_FALSE(map.is_solid({2, 2}));

        // In-bounds solid tiles
        CHECK(map.is_solid({2, 0}));
        CHECK(map.is_solid({1, 1}));

        // Out-of-bounds queries should be solid (treated as boundary walls)
        CHECK(map.is_solid({-1, 0}));
        CHECK(map.is_solid({0, -1}));
        CHECK(map.is_solid({3, 0}));
        CHECK(map.is_solid({0, 3}));
        CHECK(map.is_solid({100, 100}));
    }

    SECTION("check_collision with degenerate AABB returns false") {
        CHECK_FALSE(map.check_collision(
            AABB{glm::vec2{0.0f, 0.0f}, glm::vec2{0.0f, 0.0f}}));
        CHECK_FALSE(map.check_collision(
            AABB{glm::vec2{16.0f, 16.0f}, glm::vec2{-10.0f, 10.0f}}));
        CHECK_FALSE(map.check_collision(
            AABB{glm::vec2{16.0f, 16.0f}, glm::vec2{10.0f, -5.0f}}));
    }

    SECTION("check_collision inside empty area returns false") {
        // Tile (0, 0) is [0, 16) x [0, 16)
        AABB box{glm::vec2{2.0f, 2.0f}, glm::vec2{12.0f, 12.0f}};
        CHECK_FALSE(map.check_collision(box));
    }

    SECTION("check_collision overlapping solid tile returns true") {
        // Tile (1, 1) is [16, 32) x [16, 32) (solid)
        AABB box_inside{glm::vec2{18.0f, 18.0f}, glm::vec2{10.0f, 10.0f}};
        CHECK(map.check_collision(box_inside));

        // Straddling from empty tile (0, 1) into solid tile (1, 1)
        AABB box_straddling{glm::vec2{10.0f, 18.0f}, glm::vec2{10.0f, 10.0f}};
        CHECK(map.check_collision(box_straddling));
    }

    SECTION("check_collision with map boundary returns true") {
        // Touching outside left
        AABB box_left{glm::vec2{-2.0f, 5.0f}, glm::vec2{10.0f, 10.0f}};
        CHECK(map.check_collision(box_left));

        // Touching outside right
        AABB box_right{glm::vec2{40.0f, 5.0f}, glm::vec2{10.0f, 10.0f}};
        CHECK(map.check_collision(box_right));
    }

    SECTION("Map without collision layer treats in-bounds as non-solid") {
        MapData empty_data{};
        empty_data.map_size = {2, 2};
        empty_data.tile_size = {16, 16};
        Map empty_map(std::move(empty_data));

        CHECK_FALSE(empty_map.is_solid({0, 0}));
        CHECK_FALSE(empty_map.is_solid({1, 1}));
        CHECK(empty_map.is_solid({-1, 0}));
        CHECK_FALSE(empty_map.check_collision(
            AABB{glm::vec2{0.0f, 0.0f}, glm::vec2{16.0f, 16.0f}}));
    }
}

TEST_CASE("StringHash and StringEqual transparent operations",
          "[base][string_hash]") {
    StringHash hasher;
    StringEqual equal;

    std::string str = "serenkai_layer";
    std::string_view sv = "serenkai_layer";

    CHECK(hasher(str) == hasher(sv));
    CHECK(equal(str, sv));
    CHECK(equal(sv, str));
    CHECK_FALSE(equal(str, "different_layer"));
}
