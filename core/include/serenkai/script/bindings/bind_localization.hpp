#pragma once

struct lua_State;

namespace serenkai {

class LocalizationManager;
void bind_localization_to_lua(lua_State* L, LocalizationManager* mgr);
} // namespace serenkai