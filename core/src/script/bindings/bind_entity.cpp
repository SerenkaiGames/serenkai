#include "serenkai/script/bindings/bind_entity.hpp"

#include "serenkai/game/entity_manager.hpp"
#include "serenkai/script/lua_header.hpp"

namespace serenkai {
void register_lua_entity(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginClass<EntityManager>("EntityManager")
        .addStaticProperty("NULL_ID", &EntityManager::NULL_ID)
        .addFunction("create", &EntityManager::create)
        .addFunction("destroy", &EntityManager::destroy)
        .addFunction("alive", &EntityManager::alive)
        .addFunction("get_prop", &EntityManager::get_prop)
        .addFunction("set_prop", &EntityManager::set_prop)
        .addFunction("has_prop", &EntityManager::has_prop)
        .addFunction("remove_prop", &EntityManager::remove_prop)
        .addFunction("alive_size", &EntityManager::alive_size)
        .endClass();
}
} // namespace serenkai