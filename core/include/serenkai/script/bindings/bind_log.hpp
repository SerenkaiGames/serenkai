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
void bind_log_to_lua(lua_State* L);
} // namespace serenkai
