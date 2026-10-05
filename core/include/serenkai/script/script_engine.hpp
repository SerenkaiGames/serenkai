#pragma once

#include "serenkai/base/raii.hpp"

#include <functional>
#include <memory>

struct lua_State;

namespace serenkai {
using LuaState = RaiiWrapper<lua_State*, std::function<void(lua_State*)>,
                             std::function<void(lua_State*)>>;
/// @brief Lua scripting engine
///
/// Automatically manages the lifecycle of lua_State.
class ScriptEngine {
public:
    ScriptEngine();

private:
    std::unique_ptr<LuaState> m_lua_state;
};
} // namespace serenkai