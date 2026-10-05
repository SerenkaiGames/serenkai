#pragma once

#include "serenkai/base/raii.hpp"
#include "serenkai/resource/resource_location.hpp"
// clang-format off
#include <lua.h>
#include <lualib.h>
#include <luabridge3/LuaBridge/LuaBridge.h>
// clang-format on
#include <optional>
#include <spdlog/spdlog.h>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
struct lua_State;

namespace serenkai {

namespace lua_detail {
/// @brief Calls a Lua callable object and returns the results as a std::tuple.
/// @tparam RetTypes Types of the return values in order.
/// @tparam Args Argument types forwarded to the Lua function.
/// @param func The Lua callable object (function or object with __call
/// metamethod).
/// @param args Arguments to pass to the Lua function.
/// @return std::optional<std::tuple<RetTypes...>> containing the return values,
/// or std::nullopt on failure.
template <typename... RetTypes, typename... Args>
std::optional<std::tuple<RetTypes...>>
call_function(const luabridge::LuaRef& func, Args&&... args) {
    using ReturnTuple = std::tuple<RetTypes...>;

    if (!func.isCallable()) {
        spdlog::error("Lua object is not callable");
        return std::nullopt;
    }

    auto result = func.call<ReturnTuple>(std::forward<Args>(args)...);

    if (!result) {
        spdlog::error("Lua function call failed: {}", result.message());
        return std::nullopt;
    }

    return *result;
}

} // namespace lua_detail

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

    template <typename... RetTypes, typename... Args>
    std::optional<std::tuple<RetTypes...>>
    call(std::string_view loc, std::string_view global, Args&&... args) {
        auto fun = get_global(loc, global);
        if (!fun) {
            return std::nullopt;
        }

        return lua_detail::call_function<RetTypes...>(
            *fun, std::forward<Args>(args)...);
    }

private:
    AssetManager* m_asset_manager = nullptr;
    LuaState m_root_state;
    std::unordered_map<ResourceLocation, int> m_thread_refs;
    std::unordered_map<ResourceLocation, lua_State*> m_states;

    /// @brief Get a global variable; if the name is incorrect, it will be
    /// nullopt.
    std::optional<luabridge::LuaRef> get_global(std::string_view loc,
                                                std::string_view global);
};
} // namespace serenkai