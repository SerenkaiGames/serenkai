#include "serenkai/script/bindings/bind_entity.hpp"

#include "serenkai/game/entity.hpp"
#include "serenkai/game/entity_manager.hpp"
#include "serenkai/game/map_manager.hpp"
#include "serenkai/script/bindings/facade_wrapper.hpp"
#include "serenkai/script/bindings/luau_prop.hpp"
#include "serenkai/script/lua_header.hpp"
#include "serenkai/script/script_engine.hpp"

#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <string_view>
#include <variant>

#define BIND(name, ...)                                                        \
    addFunction(name, wrap_facade(get_subsystem, __VA_ARGS__))

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
        .BIND("play_animation",
              [](EntityManager* self, double id, std::string_view clip) {
                  return self->play_animation(static_cast<EntityId>(id), clip);
              })
        .BIND("stop_animation",
              [](EntityManager* self, double id) {
                  return self->stop_animation(static_cast<EntityId>(id));
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
        .BIND("move",
              [engine](EntityManager* self, double id, glm::vec3 dist) {
                  if (!engine) {
                      return;
                  }
                  auto ctx = engine->world_context();
                  if (!ctx.map_mgr) {
                      return;
                  }
                  auto map = ctx.map_mgr->current_map();
                  if (!map) {
                      return;
                  }
                  EntityId e = static_cast<EntityId>(id);
                  auto cur_pos = self->get_prop_by_loc(e, "props/pos"_rl);
                  if (!cur_pos) {
                      return;
                  }

                  auto p = std::get_if<glm::vec3>(&cur_pos.value());
                  if (!p) {
                      return;
                  }

                  auto col_prop = self->get_prop_by_loc(e, "props/collider"_rl);

                  glm::vec2 offset{0.0f};
                  glm::vec2 box_size{16.0f, 16.0f};

                  if (!col_prop) {
                      return;
                  }

                  if (auto* c = std::get_if<glm::vec4>(&col_prop.value())) {
                      offset = {c->x, c->y};
                      box_size = {c->z, c->w};
                  }

                  auto get_world_aabb = [&](const glm::vec3& p) {
                      return AABB{glm::vec2{p.x + offset.x, p.y + offset.y},
                                  box_size};
                  };

                  auto new_pos = *p;
                  new_pos.x += dist.x;
                  if (map->check_collision(get_world_aabb(new_pos))) {
                      new_pos.x -= dist.x;
                  }

                  new_pos.y += dist.y;
                  if (map->check_collision(get_world_aabb(new_pos))) {
                      new_pos.y -= dist.y;
                  }

                  self->set_prop_by_loc(e, "props/pos"_rl, new_pos);
              })
        .endNamespace()
        .endNamespace();
}
} // namespace serenkai