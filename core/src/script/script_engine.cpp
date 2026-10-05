#include "serenkai/script/script_engine.hpp"

#include "serenkai/script/bindings/bind_log.hpp"

#include <fstream>
#include <lua.h>
#include <luacode.h>
#include <lualib.h>
#include <spdlog/spdlog.h>
#include <stdexcept>
namespace {
void init_lua(lua_State*& L) {
    L = luaL_newstate();
    if (!L) {
        throw std::runtime_error("Failed to create lua state");
    }

    luaL_openlibs(L);
}

void cleanup_lua(lua_State*& L) { lua_close(L); }

lua_CompileOptions get_lua_options() {
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

} // namespace

namespace serenkai {
ScriptEngine::ScriptEngine() : m_state(init_lua, cleanup_lua) {

    register_lua_log(m_state.get());
}

bool ScriptEngine::run_string(const std::string& script,
                              const std::string& chunk_name) {
    size_t bytecode_size = 0;

    auto options = get_lua_options();

    char* bytecode =
        luau_compile(script.data(), script.size(), &options, &bytecode_size);
    if (!bytecode) {
        spdlog::error("luau_compile failed for chunk: {}", chunk_name);
        return false;
    }

    int load_result = luau_load(m_state.get(), chunk_name.c_str(), bytecode,
                                bytecode_size, 0);
    free(bytecode);

    if (load_result != 0) {
        const char* err = lua_tostring(m_state.get(), -1);
        spdlog::error("luau_load failed: {}", err ? err : "Unknown error");
        lua_pop(m_state.get(), 1);
        return false;
    }

    if (lua_pcall(m_state.get(), 0, 0, 0) != LUA_OK) {
        const char* err = lua_tostring(m_state.get(), -1);
        spdlog::error("Lua runtime error: {}", err ? err : "Unknown error");
        lua_pop(m_state.get(), 1);
        return false;
    }

    return true;
}

bool ScriptEngine::run_file(const std::string& path) {

    std::ifstream file(path, std::ios::binary);
    if (!file) {
        spdlog::error("Cannot open script file: {}", path);
        return false;
    }

    std::ostringstream oss;
    oss << file.rdbuf();
    return run_string(oss.str(), path);
}

} // namespace serenkai