#include "serenkai/script/bindings/bind_camera.hpp"

#include "serenkai/game/camera.hpp"
#include "serenkai/script/bindings/facade_wrapper.hpp"
#include "serenkai/script/bindings/luau_glm.hpp"
#include "serenkai/script/lua_header.hpp"
#include "serenkai/script/script_engine.hpp"

#define BIND(name, fn) addFunction(name, wrap_facade(get_subsystem, fn))

namespace serenkai {
void bind_camera_to_lua(lua_State* L, ScriptEngine* engine) {
    auto get_subsystem = [engine]() -> Camera* {
        if (!engine) {
            return nullptr;
        }
        return engine->world_context().camera;
    };
    luabridge::getGlobalNamespace(L)
        .beginNamespace("World")
        .beginNamespace("Camera")
        .BIND("pos", &Camera::pos)
        .BIND("zoom", &Camera::zoom)
        .BIND("set_pos", &Camera::set_pos)
        .BIND("move", &Camera::move)
        .BIND("set_zoom", &Camera::set_zoom)
        .BIND("add_zoom", &Camera::add_zoom)
        .endNamespace()
        .endNamespace();
}
} // namespace serenkai