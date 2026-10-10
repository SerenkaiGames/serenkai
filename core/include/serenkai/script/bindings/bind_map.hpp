#pragma once
struct lua_State;
namespace serenkai {

/// @brief Register Map, MapManager, MapObject, and Tile bindings for Luau.
void register_lua_map(lua_State* L);

} // namespace serenkai