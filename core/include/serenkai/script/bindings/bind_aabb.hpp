#pragma once
struct lua_State;

namespace serenkai {
void bind_aabb_to_lua(lua_State* L);
}