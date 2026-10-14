#pragma once
struct lua_State;

namespace serenkai {
class ScriptEngine;
/// @brief Inject the world-related variables into the global World table.
///
/// If the world has not been created, Lua will get nil.
void bind_world_to_lua(lua_State* L, ScriptEngine* engine);

} // namespace serenkai