#include "serenkai/script/bindings/bind_input.hpp"

#include "serenkai/application/input_manager.hpp"
#include "serenkai/script/lua_header.hpp"

#include <luabridge3/LuaBridge/detail/Namespace.h>
#include <stdexcept>

namespace serenkai {
void register_lua_input(lua_State* L, InputManager* input_manager) {
    if (!input_manager) {
        throw std::runtime_error(
            "Register lua input failed, input manager is nullptr");
    }

    auto action_table = luabridge::newTable(L);
    for (const auto& [name, id] : input_manager->get_action_id_map()) {
        action_table[name] = id;
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
            "get_held_mask",
            [input_manager]() { return input_manager->get_held_mask(); })
        .endNamespace();
}
} // namespace serenkai