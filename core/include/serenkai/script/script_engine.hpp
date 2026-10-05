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

    /// @brief Runs a Lua script from a string.
    /// @return true on success, false on error (error is logged).
    bool run_string(const std::string& script);

    /// @brief Runs a Lua script from a file.
    /// @return true on success, false on error (error is logged).
    bool run_file(const std::string& path);

private:
    std::unique_ptr<LuaState> m_state;
};
} // namespace serenkai