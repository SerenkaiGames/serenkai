#pragma once
struct lua_State;
namespace serenkai {

/// @brief Register Map, MapManager, MapObject, and Tile bindings for Luau.
void bind_map_to_lua(lua_State* L);

} // namespace serenkai