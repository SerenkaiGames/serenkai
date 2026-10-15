#include "serenkai/game/entity_manager.hpp"

#include <catch2/catch_test_macros.hpp>
#include <glm/vec3.hpp>
#include <string>
#include <vector>

using namespace serenkai;

TEST_CASE("EntityManager creation and destruction lifecycle",
          "[game][entity]") {
    EntityManager manager;

    SECTION("Initial state") {
        CHECK(manager.alive_size() == 0);
        CHECK_FALSE(manager.alive(EntityManager::NULL_ID));
        CHECK_FALSE(manager.alive(99999ULL));
    }

    SECTION("Entity creation and alive query") {
        EntityId e1 = manager.create();
        CHECK(e1 != EntityManager::NULL_ID);
        CHECK(manager.alive(e1));
        CHECK(manager.alive_size() == 1);

        EntityId e2 = manager.create();
        CHECK(e2 != EntityManager::NULL_ID);
        CHECK(e1 != e2);
        CHECK(manager.alive(e1));
        CHECK(manager.alive(e2));
        CHECK(manager.alive_size() == 2);
    }

    SECTION("Entity destruction") {
        EntityId e1 = manager.create();
        EntityId e2 = manager.create();

        manager.destroy(e1);
        CHECK_FALSE(manager.alive(e1));
        CHECK(manager.alive(e2));
        CHECK(manager.alive_size() == 1);

        // Double destruction is safe and a no-op
        manager.destroy(e1);
        CHECK_FALSE(manager.alive(e1));
        CHECK(manager.alive_size() == 1);

        // Destroying null or non-existent entity is safe
        manager.destroy(EntityManager::NULL_ID);
        manager.destroy(12345678ULL);
        CHECK(manager.alive_size() == 1);

        manager.destroy(e2);
        CHECK_FALSE(manager.alive(e2));
        CHECK(manager.alive_size() == 0);
    }
}

TEST_CASE("EntityManager generational indexing and ID reuse",
          "[game][entity]") {
    EntityManager manager;

    EntityId e1 = manager.create();
    CHECK(manager.alive(e1));

    manager.destroy(e1);
    CHECK_FALSE(manager.alive(e1));

    // Reusing the freed slot should increment generation and generate a new ID
    EntityId e2 = manager.create();
    CHECK(manager.alive(e2));
    CHECK(e1 != e2);
    CHECK_FALSE(manager.alive(e1));

    // Create multiple entities and destroy one in the middle
    EntityId a = manager.create();
    EntityId b = manager.create();
    EntityId c = manager.create();
    REQUIRE(manager.alive_size() == 4);

    manager.destroy(b);
    CHECK_FALSE(manager.alive(b));
    CHECK(manager.alive(a));
    CHECK(manager.alive(c));
    CHECK(manager.alive(e2));

    EntityId d = manager.create();
    CHECK(manager.alive(d));
    CHECK(d != b);
    CHECK_FALSE(manager.alive(b));
}

TEST_CASE("EntityManager property access with ResourceLocation",
          "[game][entity]") {
    EntityManager manager;
    EntityId e = manager.create();

    const auto bool_key = *ResourceLocation::parse("game:is_active");
    const auto int_key = *ResourceLocation::parse("game:hp");
    const auto float_key = *ResourceLocation::parse("game:speed");
    const auto string_key = *ResourceLocation::parse("game:name");
    const auto vec2_key = *ResourceLocation::parse("game:velocity");
    const auto vec3_key = *ResourceLocation::parse("game:pos3d");
    const auto vec4_key = *ResourceLocation::parse("game:color");

    SECTION("Set and get properties of different variant types") {
        CHECK(manager.set_prop_by_loc(e, bool_key, true));
        CHECK(manager.set_prop_by_loc(e, int_key, 100));
        CHECK(manager.set_prop_by_loc(e, float_key, 5.5f));
        CHECK(manager.set_prop_by_loc(e, string_key, std::string("Hero")));
        CHECK(manager.set_prop_by_loc(e, vec2_key, glm::vec2{1.0f, 2.0f}));
        CHECK(
            manager.set_prop_by_loc(e, vec3_key, glm::vec3{1.0f, 2.0f, 3.0f}));
        CHECK(manager.set_prop_by_loc(e, vec4_key,
                                      glm::vec4{1.0f, 0.5f, 0.2f, 1.0f}));

        CHECK(manager.has_prop_by_loc(e, bool_key));
        CHECK(manager.has_prop_by_loc(e, int_key));
        CHECK(manager.has_prop_by_loc(e, float_key));
        CHECK(manager.has_prop_by_loc(e, string_key));
        CHECK(manager.has_prop_by_loc(e, vec2_key));
        CHECK(manager.has_prop_by_loc(e, vec3_key));
        CHECK(manager.has_prop_by_loc(e, vec4_key));

        auto bool_val = manager.get_prop_by_loc(e, bool_key);
        REQUIRE(bool_val.has_value());
        CHECK(std::get<bool>(*bool_val) == true);

        auto int_val = manager.get_prop_by_loc(e, int_key);
        REQUIRE(int_val.has_value());
        CHECK(std::get<int>(*int_val) == 100);

        auto float_val = manager.get_prop_by_loc(e, float_key);
        REQUIRE(float_val.has_value());
        CHECK(std::get<float>(*float_val) == 5.5f);

        auto string_val = manager.get_prop_by_loc(e, string_key);
        REQUIRE(string_val.has_value());
        CHECK(std::get<std::string>(*string_val) == "Hero");

        auto vec2_val = manager.get_prop_by_loc(e, vec2_key);
        REQUIRE(vec2_val.has_value());
        CHECK(std::get<glm::vec2>(*vec2_val) == glm::vec2{1.0f, 2.0f});

        auto vec3_val = manager.get_prop_by_loc(e, vec3_key);
        REQUIRE(vec3_val.has_value());
        CHECK(std::get<glm::vec3>(*vec3_val) == glm::vec3{1.0f, 2.0f, 3.0f});

        auto vec4_val = manager.get_prop_by_loc(e, vec4_key);
        REQUIRE(vec4_val.has_value());
        CHECK(std::get<glm::vec4>(*vec4_val) ==
              glm::vec4{1.0f, 0.5f, 0.2f, 1.0f});
    }

    SECTION("Query non-existent property") {
        const auto unknown_key = *ResourceLocation::parse("game:unknown");
        CHECK_FALSE(manager.has_prop_by_loc(e, unknown_key));
        CHECK_FALSE(manager.get_prop_by_loc(e, unknown_key).has_value());
    }

    SECTION("Overwrite property value") {
        manager.set_prop_by_loc(e, int_key, 100);
        manager.set_prop_by_loc(e, int_key, 250);

        auto val = manager.get_prop_by_loc(e, int_key);
        REQUIRE(val.has_value());
        CHECK(std::get<int>(*val) == 250);
    }

    SECTION("Remove property") {
        manager.set_prop_by_loc(e, int_key, 100);
        CHECK(manager.has_prop_by_loc(e, int_key));

        CHECK(manager.remove_prop_by_loc(e, int_key));
        CHECK_FALSE(manager.has_prop_by_loc(e, int_key));
        CHECK_FALSE(manager.get_prop_by_loc(e, int_key).has_value());

        // Removing non-existent property returns false
        CHECK_FALSE(manager.remove_prop_by_loc(e, int_key));
    }

    SECTION("Operations on invalid or destroyed entity fail") {
        CHECK_FALSE(
            manager.set_prop_by_loc(EntityManager::NULL_ID, int_key, 10));
        CHECK_FALSE(manager.get_prop_by_loc(EntityManager::NULL_ID, int_key)
                        .has_value());
        CHECK_FALSE(manager.has_prop_by_loc(EntityManager::NULL_ID, int_key));
        CHECK_FALSE(
            manager.remove_prop_by_loc(EntityManager::NULL_ID, int_key));

        manager.set_prop_by_loc(e, int_key, 10);
        manager.destroy(e);

        CHECK_FALSE(manager.set_prop_by_loc(e, int_key, 20));
        CHECK_FALSE(manager.get_prop_by_loc(e, int_key).has_value());
        CHECK_FALSE(manager.has_prop_by_loc(e, int_key));
        CHECK_FALSE(manager.remove_prop_by_loc(e, int_key));
    }

    SECTION("Reused entity slot starts with cleared properties") {
        manager.set_prop_by_loc(e, int_key, 42);
        manager.destroy(e);

        EntityId new_e = manager.create();
        CHECK_FALSE(manager.has_prop_by_loc(new_e, int_key));
        CHECK_FALSE(manager.get_prop_by_loc(new_e, int_key).has_value());
    }
}

TEST_CASE("EntityManager property access with string_view overload",
          "[game][entity]") {
    EntityManager manager;
    EntityId e = manager.create();

    SECTION("Valid string locations") {
        CHECK(manager.set_prop(e, "game:score", 500));
        CHECK(manager.has_prop(e, "game:score"));

        auto val = manager.get_prop(e, "game:score");
        REQUIRE(val.has_value());
        CHECK(std::get<int>(*val) == 500);

        // Also readable via parsed ResourceLocation
        auto key = *ResourceLocation::parse("game:score");
        CHECK(manager.has_prop_by_loc(e, key));

        CHECK(manager.remove_prop(e, "game:score"));
        CHECK_FALSE(manager.has_prop(e, "game:score"));
    }

    SECTION("Default namespace when colon is omitted") {
        CHECK(manager.set_prop(e, "level", 3));
        CHECK(manager.has_prop(e, "level"));

        auto val = manager.get_prop(e, "serenkai:level");
        REQUIRE(val.has_value());
        CHECK(std::get<int>(*val) == 3);
    }

    SECTION("Malformed string locations fail gracefully") {
        CHECK_FALSE(manager.set_prop(e, "invalid:too:many:colons", 1));
        CHECK_FALSE(manager.set_prop(e, "", 1));
        CHECK_FALSE(manager.set_prop(e, "/starts_with_slash", 1));

        CHECK_FALSE(manager.has_prop(e, "invalid:too:many:colons"));
        CHECK_FALSE(manager.has_prop(e, ""));

        CHECK_FALSE(manager.get_prop(e, "invalid:too:many:colons").has_value());
        CHECK_FALSE(manager.get_prop(e, "").has_value());

        CHECK_FALSE(manager.remove_prop(e, "invalid:too:many:colons"));
        CHECK_FALSE(manager.remove_prop(e, ""));
    }
}

TEST_CASE("EntityManager for_each_alive iteration", "[game][entity]") {
    EntityManager manager;

    EntityId e1 = manager.create();
    EntityId e2 = manager.create();
    EntityId e3 = manager.create();

    manager.set_prop(e1, "index", 1);
    manager.set_prop(e2, "index", 2);
    manager.set_prop(e3, "index", 3);

    manager.destroy(e2);

    std::vector<EntityId> visited_ids;
    std::vector<int> visited_indices;

    manager.for_each_alive([&](EntityId id, Entity& entity) {
        visited_ids.push_back(id);
        auto key = *ResourceLocation::parse("index");
        auto it = entity.props.find(key);
        if (it != entity.props.end()) {
            visited_indices.push_back(std::get<int>(it->second));
        }
    });

    REQUIRE(visited_ids.size() == 2);
    CHECK(visited_ids[0] == e1);
    CHECK(visited_ids[1] == e3);

    REQUIRE(visited_indices.size() == 2);
    CHECK(visited_indices[0] == 1);
    CHECK(visited_indices[1] == 3);
}

TEST_CASE("Entity get_prop template helper", "[game][entity]") {
    EntityManager manager;
    EntityId e = manager.create();

    manager.set_prop(e, "speed", 5.5f);
    manager.set_prop(e, "tag", std::string("player"));
    manager.set_prop(e, "pos", glm::vec3{10.0f, 20.0f, 1.0f});

    manager.for_each_alive([&](EntityId, const Entity& entity) {
        auto speed = entity.get_prop<float>(*ResourceLocation::parse("speed"));
        REQUIRE(speed != nullptr);
        CHECK(*speed == 5.5f);

        auto tag =
            entity.get_prop<std::string>(*ResourceLocation::parse("tag"));
        REQUIRE(tag != nullptr);
        CHECK(*tag == "player");

        auto pos = entity.get_prop<glm::vec3>(*ResourceLocation::parse("pos"));
        REQUIRE(pos != nullptr);
        CHECK(pos->x == 10.0f);
        CHECK(pos->y == 20.0f);
        CHECK(pos->z == 1.0f);

        // Non-existent prop returns nullptr
        auto missing =
            entity.get_prop<int>(*ResourceLocation::parse("missing"));
        CHECK(missing == nullptr);

        // Type mismatch returns nullptr
        auto wrong_type =
            entity.get_prop<int>(*ResourceLocation::parse("speed"));
        CHECK(wrong_type == nullptr);
    });
}
