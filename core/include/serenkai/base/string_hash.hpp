#pragma once

#include <string_view>
namespace serenkai {
struct StringHash {
    using is_transparent = void; // NOLINT

    std::size_t operator()(std::string_view sv) const noexcept {
        return std::hash<std::string_view>{}(sv);
    }
};

struct StringEqual {
    using is_transparent = void; // NOLINT

    bool operator()(std::string_view a, std::string_view b) const noexcept {
        return a == b;
    }
};

} // namespace serenkai