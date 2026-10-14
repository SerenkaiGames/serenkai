#pragma once
#include "serenkai/game/property.hpp"
#include "serenkai/script/bindings/luau_glm.hpp"
#include "serenkai/script/lua_header.hpp"

#include <type_traits>
namespace serenkai {
inline void push_property(lua_State* L, const PropValue& prop) {

    std::visit(
        [L](const auto& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                lua_pushnil(L);
            } else if constexpr (std::is_same_v<T, bool>) {
                lua_pushboolean(L, v ? 1 : 0);
            } else if constexpr (std::is_same_v<T, int>) {
                lua_pushinteger(L, v);
            } else if constexpr (std::is_same_v<T, float>) {
                lua_pushnumber(L, v);
            } else if constexpr (std::is_same_v<T, std::string>) {
                lua_pushlstring(L, v.data(), v.size());
            } else if constexpr (std::is_same_v<T, glm::vec4>) {

#if LUA_VECTOR_SIZE == 4
                lua_pushvector(L, v.x, v.y, v.z, v.w);
#else
                lua_pushvector(L, v.x, v.y, v.z);
#endif
            } else if constexpr (std::is_same_v<T, glm::vec2>) {
                lua_pushvector(L, v.x, v.y, 0);
            } else if constexpr (std::is_same_v<T, glm::vec3>) {
                lua_pushvector(L, v.x, v.y, v.z);
            }
        },
        prop);
}
} // namespace serenkai