#include "serenkai/script/script_engine.hpp"

#include "serenkai/script/bindings/bind_log.hpp"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <memory>
#include <spdlog/spdlog.h>
#include <stdexcept>
namespace {
void init_lua(lua_State* L) {
    L = luaL_newstate();
    if (!L) {
        throw std::runtime_error("Failed to create lua state");
    }

    luaL_openlibs(L);
}

void cleanup_lua(lua_State* L) { lua_close(L); }

} // namespace

namespace serenkai {
ScriptEngine::ScriptEngine() {
    m_state = std::make_unique<LuaState>(init_lua, cleanup_lua);
    register_lua_log(m_state->get());
}

bool ScriptEngine::run_string(const std::string& script) {
    if (luaL_dostring(m_state->get(), script.c_str()) != LUA_OK) {
        spdlog::error("Lua error: {}", lua_tostring(m_state->get(), -1));
        lua_pop(m_state->get(), 1);
        return false;
    }
    return true;
}

bool ScriptEngine::run_file(const std::string& path) {
    if (luaL_dofile(m_state->get(), path.c_str()) != LUA_OK) {
        spdlog::error("Lua error: {}", lua_tostring(m_state->get(), -1));
        lua_pop(m_state->get(), 1);
        return false;
    }
    return true;
}

} // namespace serenkai