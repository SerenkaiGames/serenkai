#include "serenkai/script/bindings/bind_log.hpp"

#include "serenkai/script/lua_header.hpp"

#include <spdlog/spdlog.h>
#include <sstream>
#include <string>
namespace {

std::string lua_args_to_string(lua_State* state) {
    int n = lua_gettop(state);
    std::ostringstream oss;
    for (int i = 1; i <= n; ++i) {
        if (i > 1) {
            oss << '\t';
        }
        const char* s = luaL_tolstring(state, i, nullptr);
        oss << (s ? s : "(null)");
        lua_pop(state, 1);
    }
    return oss.str();
}

int lua_print(lua_State* state) {
    spdlog::info("{}", lua_args_to_string(state));
    return 0;
}

int lua_log_info(lua_State* state) {
    spdlog::info("{}", lua_args_to_string(state));
    return 0;
}

int lua_log_warn(lua_State* state) {
    spdlog::warn("{}", lua_args_to_string(state));
    return 0;
}

int lua_log_error(lua_State* state) {
    spdlog::error("{}", lua_args_to_string(state));
    return 0;
}

int lua_log_debug(lua_State* state) {
    spdlog::debug("{}", lua_args_to_string(state));
    return 0;
}

} // namespace

namespace serenkai {
void register_lua_log(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .addFunction("print", lua_print)
        .beginNamespace("log")
        .addFunction("info", lua_log_info)
        .addFunction("warn", lua_log_warn)
        .addFunction("error", lua_log_error)
        .addFunction("debug", lua_log_debug)
        .endNamespace();
}
} // namespace serenkai