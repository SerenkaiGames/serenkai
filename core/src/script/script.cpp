#include "serenkai/script/script.hpp"

#include <fmt/format.h>
#include <luacode.h>
#include <stdexcept>
#include <utility>

namespace serenkai {

lua_CompileOptions Script::get_lua_options() {
    lua_CompileOptions options{};

#ifdef NDEBUG
    // Release
    options.optimizationLevel = 2;
    options.debugLevel = 0;
#else
    // Debug
    options.optimizationLevel = 1;
    options.debugLevel = 2;
#endif
    options.typeInfoLevel = 1;

    return options;
}

Script::Script(LuaState state, std::string_view script, std::string_view name)
    : m_state(std::move(state)), m_name(name) {
    load(script);
}

void Script::load(std::string_view script) {
    size_t bytecode_size = 0;

    auto options = get_lua_options();

    char* bytecode =
        luau_compile(script.data(), script.size(), &options, &bytecode_size);
    if (!bytecode) {
        throw std::runtime_error(
            fmt::format("luau_compile failed for chunk: {}", m_name));
    }

    int load_result =
        luau_load(m_state.get(), m_name.c_str(), bytecode, bytecode_size, 0);
    free(bytecode);

    if (load_result != 0) {
        const char* err = lua_tostring(m_state.get(), -1);

        lua_pop(m_state.get(), 1);
        throw std::runtime_error(
            fmt::format("luau_load failed: {}", err ? err : "Unknown error"));
    }

    if (lua_resume(m_state.get(), nullptr, 0) != LUA_OK) {
        const char* err = lua_tostring(m_state.get(), -1);
        lua_pop(m_state.get(), 1);
        throw std::runtime_error(
            fmt::format("Lua runtime error: {}", err ? err : "Unknown error"));
    }
}

std::optional<luabridge::LuaRef> Script::get_global(std::string_view global) {
    auto var = luabridge::getGlobal(m_state.get(), std::string(global).c_str());
    if (var.isNil()) {
        return std::nullopt;
    }
    return var;
}

} // namespace serenkai