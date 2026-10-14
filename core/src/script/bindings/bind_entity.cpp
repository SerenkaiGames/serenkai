#include "serenkai/script/bindings/bind_entity.hpp"

#include "serenkai/game/entity_manager.hpp"
#include "serenkai/script/bindings/luau_prop.hpp"
#include "serenkai/script/lua_header.hpp"

namespace serenkai {
namespace {
int lua_entity_set_prop(lua_State* L) {
    auto res_mgr = luabridge::Stack<EntityManager*>::get(L, 1);
    if (!res_mgr) {
        lua_pushboolean(L, 0);
        return 1;
    }
    auto* mgr = *res_mgr;

    const double id_num = luaL_checknumber(L, 2);
    const EntityId id = static_cast<EntityId>(id_num);

    size_t len = 0;
    const char* loc_str = luaL_checklstring(L, 3, &len);
    const std::string_view loc{loc_str, len};

    const int type = lua_type(L, 4);
    PropValue val;
    switch (type) {
    case LUA_TNIL:
        val = std::monostate{};
        break;
    case LUA_TBOOLEAN:
        val = (lua_toboolean(L, 4) != 0);
        break;
    case LUA_TNUMBER: {
        const double n = lua_tonumber(L, 4);

        if (static_cast<double>(static_cast<int>(n)) == n) {
            val = static_cast<int>(n);
        } else {
            val = static_cast<float>(n);
        }
        break;
    }
    case LUA_TSTRING: {
        size_t str_len = 0;
        const char* s = lua_tolstring(L, 4, &str_len);
        val = std::string(s, str_len);
        break;
    }
    case LUA_TVECTOR: {

        const float* v = lua_tovector(L, 4);
        val = glm::vec3(v[0], v[1], v[2]);
        break;
    }
    default:
        luaL_error(L, "Unsupported property value type: %s",
                   lua_typename(L, type));
        return 0;
    }

    const bool res = mgr->set_prop(id, loc, std::move(val));
    lua_pushboolean(L, res ? 1 : 0);
    return 1;
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

void bind_entity_to_lua(lua_State* L) {
    luabridge::getGlobalNamespace(L)
        .beginClass<EntityManager>("EntityManager")
        .addStaticProperty("NULL_ID", &EntityManager::NULL_ID)
        .addProperty("NULL_ID",
                     [](const EntityManager*) -> double {
                         return static_cast<double>(EntityManager::NULL_ID);
                     })
        .addFunction("create",
                     [](EntityManager* self) -> double {
                         return static_cast<double>(self->create());
                     })
        .addFunction("destroy",
                     [](EntityManager* self, double id) {
                         self->destroy(static_cast<EntityId>(id));
                     })
        .addFunction("alive",
                     [](EntityManager* self, double id) -> bool {
                         return self->alive(static_cast<EntityId>(id));
                     })
        .addFunction(
            "has_prop",
            [](EntityManager* self, double id, std::string_view loc) -> bool {
                return self->has_prop(static_cast<EntityId>(id), loc);
            })
        .addFunction(
            "remove_prop",
            [](EntityManager* self, double id, std::string_view loc) -> bool {
                return self->remove_prop(static_cast<EntityId>(id), loc);
            })
        .addFunction("alive_size", &EntityManager::alive_size)
        .addFunction("set_prop", &lua_entity_set_prop)
        .addFunction("get_prop", &lua_entity_get_prop)
        .endClass();
}
} // namespace serenkai