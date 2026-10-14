#pragma once

struct lua_State;

namespace serenkai {
class InputManager;

/// @brief Register input_manager into the global space.
/// @warning Ensure that input_manager remains alive throughout the script's
/// lifecycle.
void bind_input_to_lua(lua_State* L, InputManager* input_manager);
} // namespace serenkai