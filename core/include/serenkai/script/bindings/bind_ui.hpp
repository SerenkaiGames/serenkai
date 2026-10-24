#pragma once

struct lua_State;

namespace serenkai {

class ScriptEngine;
void bind_ui_to_lua(lua_State* L, ScriptEngine* engine);
} // namespace serenkai