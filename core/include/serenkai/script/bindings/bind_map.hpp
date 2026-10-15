#pragma once
struct lua_State;
namespace serenkai {
class ScriptEngine;
/// @brief Register Map, MapManager, MapObject, and Tile bindings for Luau.
void bind_map_to_lua(lua_State* L, ScriptEngine* engine);

} // namespace serenkai