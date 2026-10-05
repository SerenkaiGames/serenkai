#include "serenkai/script/script_engine.hpp"

#include "serenkai/script/bindings/bind_log.hpp"

#include <lauxlib.h>
#include <lua.h>
#include <lualib.h>
#include <memory>
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
    m_lua_state = std::make_unique<LuaState>(init_lua, cleanup_lua);
    register_lua_log(m_lua_state->get());
}
} // namespace serenkai