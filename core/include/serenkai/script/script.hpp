#pragma once
#include "serenkai/base/raii.hpp"

// clang-format off
#include <functional>
#include <lua.h>
#include <lualib.h>
#include <luabridge3/LuaBridge/LuaBridge.h>
// clang-format on

#include <spdlog/spdlog.h>
#include <string>
#include <string_view>

struct lua_CompileOptions;

namespace serenkai {
using LuaState = RaiiWrapper<lua_State*, std::function<void(lua_State*&)>,
                             std::function<void(lua_State*&)>>;
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

/// @brief Script class
///
/// Runs and loads the script once during initialization.
class Script {
public:
    Script(const Script&) = delete;
    Script(Script&&) noexcept = default;
    Script& operator=(const Script&) = delete;
    Script& operator=(Script&&) noexcept = default;

    /// @note May throws an exception.
    Script(LuaState state, std::string_view script, std::string_view name);
    ~Script() = default;

    static lua_CompileOptions get_lua_options();

    template <typename... RetTypes, typename... Args>
    std::optional<std::tuple<RetTypes...>> call(std::string_view func,
                                                Args&&... args) {
        auto fun = get_global(func);
        if (!fun) {
            return std::nullopt;
        }

        return lua_detail::call_function<RetTypes...>(
            *fun, std::forward<Args>(args)...);
    }

private:
    LuaState m_state;
    std::string m_name;

    /// @brief Get the global variable of the current script; if it is nil,
    /// return std::nullopt.
    std::optional<luabridge::LuaRef> get_global(std::string_view global);

    /// @brief Load bytecode and initialize.May throws an exception.
    void load(std::string_view script);
};
} // namespace serenkai