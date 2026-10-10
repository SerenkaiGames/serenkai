
#pragma once
// IWYU pragma: always_keep
#include "serenkai/script/lua_header.hpp"

#include <cmath>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <limits>
#include <type_traits>

namespace luabridge::luau_glm_detail {

template <class T, class Component, glm::length_t N> struct VectorStack {
    static Result push(lua_State* L, const T& value) {
        const float x = static_cast<float>(value[0]);
        const float y = static_cast<float>(value[1]);

        float z = 0.0f;

        if constexpr (N == 3) {
            z = static_cast<float>(value[2]);
        }

#if LUA_VECTOR_SIZE == 4
        lua_pushvector(L, x, y, z, 0.0f);
#else
        lua_pushvector(L, x, y, z);
#endif

        return {};
    }

    static TypeResult<T> get(lua_State* L, int index) {
        const float* components = lua_tovector(L, index);

        if (components == nullptr) {
            return makeErrorCode(ErrorCode::InvalidTypeCast);
        }

        T result{};

        for (glm::length_t i = 0; i < N; ++i) {
            const float component = components[i];

            if constexpr (std::is_integral_v<Component>) {
                const double value = static_cast<double>(component);
                constexpr double min = static_cast<double>(
                    std::numeric_limits<Component>::lowest());
                constexpr double max =
                    static_cast<double>(std::numeric_limits<Component>::max());

                if (!std::isfinite(value) || value < min || value > max) {
                    return makeErrorCode(ErrorCode::InvalidTypeCast);
                }

                // Floating-point values are truncated toward zero.
                result[i] = static_cast<Component>(component);
            } else {
                result[i] = static_cast<Component>(component);
            }
        }

        return result;
    }

    static bool is_instance(lua_State* L, int index) {
        return lua_isvector(L, index) != 0;
    }
};

} // namespace luabridge::luau_glm_detail

namespace luabridge {

template <>
struct Stack<glm::vec2> : luau_glm_detail::VectorStack<glm::vec2, float, 2> {};

template <>
struct Stack<glm::vec3> : luau_glm_detail::VectorStack<glm::vec3, float, 3> {};

template <>
struct Stack<glm::ivec2> : luau_glm_detail::VectorStack<glm::ivec2, int, 2> {};

template <>
struct Stack<glm::ivec3> : luau_glm_detail::VectorStack<glm::ivec3, int, 3> {};

} // namespace luabridge
