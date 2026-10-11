#include "serenkai/script/bindings/bind_camera.hpp"

#include "serenkai/game/camera.hpp"
#include "serenkai/script/lua_header.hpp"

#include <luabridge3/LuaBridge/detail/Namespace.h>

namespace serenkai {
void register_lua_camera(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginClass<Camera>("Camera")
        .addProperty("pos", &Camera::pos)
        .addProperty("zoom", &Camera::zoom)
        .addFunction("set_pos", &Camera::set_pos)
        .addFunction("move", &Camera::move)
        .addFunction("set_zoom", &Camera::set_zoom)
        .endClass();
}
} // namespace serenkai