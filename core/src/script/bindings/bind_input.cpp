#include "serenkai/script/bindings/bind_input.hpp"

#include "serenkai/application/input_manager.hpp"
#include "serenkai/script/lua_header.hpp"

#include <luabridge3/LuaBridge/detail/Namespace.h>
#include <spdlog/spdlog.h>

namespace serenkai {
void bind_input_to_lua(lua_State* L, InputManager* input_manager) {
    if (!input_manager) {
        spdlog::debug("Failed to bind input, input manager is nullptr");
        return;
    }

    auto action_table = luabridge::newTable(L);
    for (const auto& [name, id] : input_manager->get_action_id_map()) {
        action_table[name.str()] = id;
        // Provide a shorter path for the default namespace.
        if (name.ns() == ResourceLocation::DEFAULT_NAMESPACE) {
            action_table[name.path()] = id;
        }
    }

    luabridge::getGlobalNamespace(L)
        .beginNamespace("Input")
        .addVariable("Action", action_table)
        .addFunction("is_down",
                     [input_manager](InputManager::ActionId id) {
                         return input_manager->is_down(id);
                     })
        .addFunction("is_pressed",
                     [input_manager](InputManager::ActionId id) {
                         return input_manager->is_pressed(id);
                     })
        .addFunction("is_released",
                     [input_manager](InputManager::ActionId id) {
                         return input_manager->is_released(id);
                     })
        .addFunction(
            "get_held_actions",
            [input_manager]() { return input_manager->get_held_actions(); })
        .endNamespace();
}
} // namespace serenkai