#include "serenkai/script/bindings/bind_entity.hpp"

#include "serenkai/game/entity.hpp"
#include "serenkai/game/entity_manager.hpp"
#include "serenkai/script/bindings/facade_wrapper.hpp"
#include "serenkai/script/bindings/luau_prop.hpp"
#include "serenkai/script/lua_header.hpp"
#include "serenkai/script/script_engine.hpp"

#define BIND(name, fn) addFunction(name, wrap_facade(get_subsystem, fn))

namespace serenkai {
namespace {
PropValue lua_ref_to_prop(const luabridge::LuaRef& ref) {
    if (ref.isNil()) {
        return std::monostate{};
    }
    if (ref.isBool()) {
        return ref.cast<bool>().value();
    }
    if (ref.isNumber()) {
        double n = ref.cast<double>().value();
        if (static_cast<double>(static_cast<int>(n)) == n) {
            return static_cast<int>(n);
        }
        return static_cast<float>(n);
    }
    if (ref.isString()) {
        return ref.cast<std::string>().value();
    }

    lua_State* l = ref.state();
    ref.push();
    if (lua_isvector(l, -1)) {
        const float* v = lua_tovector(l, -1);
        glm::vec3 vec{v[0], v[1], v[2]};
        lua_pop(l, 1);
        return vec;
    }
    lua_pop(l, 1);

    return std::monostate{};
}

int lua_entity_get_prop(lua_State* L) {
    auto res_mgr = luabridge::Stack<EntityManager*>::get(L, 1);
    if (!res_mgr) {
        lua_pushnil(L);
        return 1;
    }
    auto* mgr = *res_mgr;

    const double id_num = luaL_checknumber(L, 2);
    const EntityId id = static_cast<EntityId>(id_num);

    size_t len = 0;
    const char* loc_str = luaL_checklstring(L, 3, &len);
    const std::string_view loc{loc_str, len};

    auto prop = mgr->get_prop(id, loc);
    if (!prop.has_value()) {
        lua_pushnil(L);
        return 1;
    }

    push_property(L, *prop);
    return 1;
}

} // namespace

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