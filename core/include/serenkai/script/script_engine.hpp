#pragma once

#include "serenkai/base/raii.hpp"
#include "serenkai/resource/resource_location.hpp"

#include <string>
#include <string_view>
#include <unordered_map>

struct lua_State;

namespace serenkai {
class AssetManager;
using LuaState =
    RaiiWrapper<lua_State*, void (*)(lua_State*&), void (*)(lua_State*&)>;
/// @brief Lua scripting engine
///
/// Automatically manages the lifecycle of lua_State.
class ScriptEngine {
public:
    ScriptEngine(AssetManager* asset_manager);

    /// @brief Runs a Lua script from a string.
    /// @param L Script thread instead of root lua states
    /// @return True on success, false on error (error is logged).
    bool run_string(lua_State* L, const std::string& script,
                    const std::string& chunk_name = "chunk");

    /// @brief Runs a Lua script from a file.
    /// @param L Script thread instead of root lua states
    /// @return True on success, false on error (error is logged).
    bool run_file(lua_State* L, const std::string& path);

    /// @brief Initialize and load a script into a new Lua thread.
    bool load(std::string_view loc);

    bool unload(std::string_view loc);

private:
    AssetManager* m_asset_manager = nullptr;
    LuaState m_root_state;
    std::unordered_map<ResourceLocation, int> m_thread_refs;
    std::unordered_map<ResourceLocation, lua_State*> m_states;
};
} // namespace serenkai