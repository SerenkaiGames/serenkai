#include "serenkai/script/bindings/bind_world.hpp"

#include "serenkai/script/lua_header.hpp"
#include "serenkai/script/script_engine.hpp"

namespace serenkai {
void bind_world_to_lua(lua_State* L, ScriptEngine* engine) {
    luabridge::getGlobalNamespace(L)
        .beginNamespace("World")
        .addFunction("get_map_mgr",
                     [engine]() { return engine->world_context().map_mgr; })
        .addFunction("get_entity_mgr",
                     [engine]() { return engine->world_context().entity_mgr; })
        .addFunction("get_camera",
                     [engine]() { return engine->world_context().entity_mgr; })
        .endNamespace();
}
} // namespace serenkai