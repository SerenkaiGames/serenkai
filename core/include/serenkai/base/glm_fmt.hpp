#pragma once
// IWYU pragma: always_keep
#include <fmt/format.h>
#include <glm/glm.hpp>

namespace fmt {

template <glm::length_t L, typename T, glm::qualifier Q>
struct formatter<glm::vec<L, T, Q>> {
    constexpr auto parse(format_parse_context& ctx) -> decltype(ctx.begin()) {
        auto it = ctx.begin();
        if (it != ctx.end() && *it != '}') {
            throw format_error("invalid format specifier for glm::vec");
        }
        return it;
    }

    template <typename FormatContext>
    auto format(const glm::vec<L, T, Q>& v, FormatContext& ctx) const
        -> decltype(ctx.out()) {
        auto out = ctx.out();
        *out++ = '(';
        for (glm::length_t i = 0; i < L; ++i) {
            if (i != 0) {
                out = fmt::format_to(out, ", ");
            }
            out = fmt::format_to(out, "{}", v[i]);
        }
        *out++ = ')';
        return out;
    }
};

} // namespace fmt