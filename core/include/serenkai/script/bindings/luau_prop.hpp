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

inline PropValue lua_ref_to_prop(const luabridge::LuaRef& ref) {
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

} // namespace serenkai