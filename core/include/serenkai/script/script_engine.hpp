#pragma once

#include "serenkai/base/raii.hpp"

#include <string>

struct lua_State;

namespace serenkai {
using LuaState =
    RaiiWrapper<lua_State*, void (*)(lua_State*&), void (*)(lua_State*&)>;
/// @brief Lua scripting engine
///
/// Automatically manages the lifecycle of lua_State.
class ScriptEngine {
public:
    ScriptEngine();

    /// @brief Runs a Lua script from a string.
    /// @return true on success, false on error (error is logged).
    bool run_string(const std::string& script,
                    const std::string& chunk_name = "chunk");

    /// @brief Runs a Lua script from a file.
    /// @return true on success, false on error (error is logged).
    bool run_file(const std::string& path);

private:
    LuaState m_state;
};
} // namespace serenkai