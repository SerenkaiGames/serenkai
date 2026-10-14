#pragma once

struct lua_State;

namespace serenkai {
void bind_entity_to_lua(lua_State* L);
}