#pragma once
// IWYU pragma: always_keep
#include <glaze/glaze.hpp>
#include <glm/glm.hpp>

template <glm::length_t L, typename T, glm::qualifier Q>
struct glz::meta<glm::vec<L, T, Q>> {
    using V = glm::vec<L, T, Q>;

    static constexpr auto value = [] { // NOLINT
        if constexpr (L == 1) {
            return glz::array(&V::x);
        } else if constexpr (L == 2) {
            return glz::array(&V::x, &V::y);
        } else if constexpr (L == 3) {
            return glz::array(&V::x, &V::y, &V::z);
        } else {
            return glz::array(&V::x, &V::y, &V::z, &V::w);
        }
    }();
};