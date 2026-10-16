#include "serenkai/script/bindings/bind_entity.hpp"

#include "serenkai/game/entity.hpp"
#include "serenkai/game/entity_manager.hpp"
#include "serenkai/script/bindings/facade_wrapper.hpp"
#include "serenkai/script/bindings/luau_prop.hpp"
#include "serenkai/script/lua_header.hpp"
#include "serenkai/script/script_engine.hpp"

#include <string_view>

#define BIND(name, fn) addFunction(name, wrap_facade(get_subsystem, fn))

namespace serenkai {

void bind_entity_to_lua(lua_State* L, ScriptEngine* engine) {

    auto get_subsystem = [engine]() -> EntityManager* {
        if (!engine) {
            return nullptr;
        }
        return engine->world_context().entity_mgr;
    };

    luabridge::getGlobalNamespace(L)
        .beginNamespace("World")
        .beginNamespace("Entities")
        .addProperty("NULL_ID",
                     []() -> double {
                         return static_cast<double>(EntityManager::NULL_ID);
                     })
        .BIND("create",
              [](EntityManager* self) -> double {
                  return static_cast<double>(self->create());
              })
        .BIND("create_with_loc",
              [](EntityManager* self, std::string_view loc) {
                  return static_cast<double>(self->create_with_loc(loc));
              })
        .BIND("destroy",
              [](EntityManager* self, double id) {
                  self->destroy(static_cast<EntityId>(id));
              })
        .BIND("alive",
              [](EntityManager* self, double id) -> bool {
                  return self->alive(static_cast<EntityId>(id));
              })
        .BIND("has_prop",
              [](EntityManager* self, double id, std::string_view loc) -> bool {
                  return self->has_prop(static_cast<EntityId>(id), loc);
              })
        .BIND("remove_prop",
              [](EntityManager* self, double id, std::string_view loc) -> bool {
                  return self->remove_prop(static_cast<EntityId>(id), loc);
              })
        .BIND("alive_size", &EntityManager::alive_size)
        .BIND("set_prop",
              [](EntityManager* self, double id, std::string_view loc,
                 luabridge::LuaRef val) -> bool {
                  return self->set_prop(static_cast<EntityId>(id), loc,
                                        lua_ref_to_prop(val));
              })
        .BIND("get_prop",
              [L](EntityManager* self, double id,
                  std::string_view loc) -> std::optional<luabridge::LuaRef> {
                  auto prop = self->get_prop(static_cast<EntityId>(id), loc);
                  if (!prop.has_value()) {
                      return std::nullopt;
                  }
                  push_property(L, *prop);
                  auto ref = luabridge::LuaRef::fromStack(L, -1);
                  lua_pop(L, 1);
                  return ref;
              })
        .endNamespace()
        .endNamespace();
}
} // namespace serenkai