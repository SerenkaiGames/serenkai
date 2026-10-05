#pragma once

struct lua_State;

namespace serenkai {
/// @brief Binds the logging library to Lua
///
/// Includes:
/// print
/// log.info
/// log.error
/// log.warn
/// log.debug
void register_lua_log(lua_State* L);
} // namespace serenkai
