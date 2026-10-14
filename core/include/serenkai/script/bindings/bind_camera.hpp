#pragma once

struct lua_State;

namespace serenkai {
void bind_camera_to_lua(lua_State* L);
}