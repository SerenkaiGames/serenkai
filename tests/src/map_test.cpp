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
