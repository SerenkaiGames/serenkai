#include "serenkai/script/bindings/bind_map.hpp"

#include "serenkai/game/map.hpp"
#include "serenkai/game/map_data.hpp"
#include "serenkai/game/map_manager.hpp"
#include "serenkai/script/bindings/facade_wrapper.hpp"
#include "serenkai/script/bindings/luau_glm.hpp"
#include "serenkai/script/bindings/luau_prop.hpp"
#include "serenkai/script/lua_header.hpp"
#include "serenkai/script/script_engine.hpp"

#include <optional>
#include <string_view>

#define BIND(name, fn) addFunction(name, wrap_facade(get_subsystem, fn))

namespace serenkai {

namespace {
int get_property(lua_State* L) {
    const auto* obj =
        luabridge::Stack<const MapObject*>::get(L, 1).valueOr(nullptr);
    if (!obj) {
        lua_pushnil(L);
        return 1;
    }

    size_t len = 0;
    const char* str = lua_tolstring(L, 2, &len);
    if (!str) {
        lua_pushnil(L);
        return 1;
    }

    const std::string_view key{str, len};
    for (const auto& prop : obj->properties) {
        if (prop.name == key) {
            push_property(L, prop.value);
            return 1;
        }
    }
    lua_pushnil(L);
    return 1;
}
} // namespace

void bind_map_to_lua(lua_State* L, ScriptEngine* engine) {

    auto get_subsystem = [engine]() -> MapManager* {
        if (!engine) {
            return nullptr;
        }
        return engine->world_context().map_mgr;
    };

    luabridge::getGlobalNamespace(L)
        .beginNamespace("World")
        .beginNamespace("Map")
        .BIND("switch_map",
              [](MapManager* self, std::string_view loc,
                 std::optional<std::string_view> spawn_point_name) {
                  return self->switch_map(loc, spawn_point_name.value_or(""));
              })
        .BIND("current_map", &MapManager::current_map)
        .BIND("has_map", &MapManager::has_map)
        .endNamespace()
        .endNamespace()

        .beginClass<Tile>("Tile")
        .addProperty("gid", &Tile::gid)
        .addProperty("local_id", &Tile::local_id)
        .addFunction("is_empty", &Tile::is_empty)
        .endClass()
        .beginClass<MapObject>("MapObject")
        .addProperty("name", &MapObject::name)
        .addProperty("type", &MapObject::type)
        .addProperty("pos", &MapObject::pos)
        .addProperty("aabb", &MapObject::aabb)
        .addProperty("visible", &MapObject::visible)
        .addFunction("get_property", &get_property)
        .endClass()
        .beginClass<Map>("Map")
        .addFunction("map_size", &Map::map_size)
        .addFunction("tile_size", &Map::tile_size)
        .addFunction("pixel_size", &Map::pixel_size)
        .addFunction("is_in_bounds", &Map::is_in_bounds)
        .addFunction("world_to_tile", &Map::world_to_tile)
        .addFunction("tile_to_world", &Map::tile_to_world)
        .addFunction("has_layer", &Map::has_layer)
        .addFunction("set_layer_visible", &Map::set_layer_visible)
        .addFunction("find_object", &Map::find_object)
        .addFunction("find_objects_by_type", &Map::find_objects_by_type)
        .addFunction("get_spawn_point", &Map::get_spawn_point)
        .addFunction("is_solid", &Map::is_solid)
        .addFunction("check_collision", &Map::check_collision)
        .endClass();
}
} // namespace serenkai