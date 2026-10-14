#include "serenkai/script/bindings/bind_entity.hpp"

#include "serenkai/game/entity_manager.hpp"
#include "serenkai/script/lua_header.hpp"

#include <luabridge3/LuaBridge/detail/Namespace.h>
#include <luabridge3/LuaBridge/detail/Overload.h>

namespace serenkai {
void register_lua_entity(lua_State* L) {
    luabridge::getNamespaceFromStack(L)
        .beginClass<EntityManager>("EntityManager")
        .addFunction("create", &EntityManager::create)
        .addFunction("destroy", &EntityManager::destroy)
        .addFunction("alive", &EntityManager::alive)
        .addFunction("get_prop", &EntityManager::get_prop)
        .addFunction("set_prop", &EntityManager::set_prop_by_loc)
        .addFunction("has_prop", &EntityManager::has_prop)
        .addFunction("remove_prop", &EntityManager::remove_prop)
        .addFunction("alive_size", &EntityManager::alive_size)
        .endClass();
}
} // namespace serenkai