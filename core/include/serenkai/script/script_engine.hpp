#pragma once

#include "serenkai/resource/resource_location.hpp"
#include "serenkai/script/script.hpp"

#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <unordered_map>

// clang-format off
#include <lua.h>
#include <lualib.h>
#include <luabridge3/LuaBridge/LuaBridge.h>
// clang-format on

namespace serenkai {

class AssetManager;

/// @brief Lua scripting engine
///
/// Automatically manages the lifecycle of lua_State.
class ScriptEngine {
public:
    ScriptEngine(AssetManager* asset_manager);

    /// @brief Create a script.
    ///
    /// @return Returns the script class; it must be used while the script
    /// engine exists.
    std::optional<Script> create(std::string_view loc);
    std::optional<Script> create(const ResourceLocation& loc);

private:
    AssetManager* m_asset_manager = nullptr;
    LuaState m_root;

    std::unordered_map<ResourceLocation, luabridge::LuaRef> m_module_cache;

    /// @brief Runs a Lua script from a string.
    /// @param L Script thread instead of root lua states
    /// @return True on success, false on error (error is logged).
    bool run_string(lua_State* L, const std::string& script,
                    const std::string& chunk_name = "chunk");

    /// @brief Runs a Lua script from a file.
    /// @param L Script thread instead of root lua states
    /// @return True on success, false on error (error is logged).
    bool run_file(lua_State* L, const std::string& path);

    /// @brief Load a module and cache the result.
    luabridge::LuaRef require(const std::string& module_name);
};
} // namespace serenkai