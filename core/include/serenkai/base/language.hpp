#pragma once

#include <cstdint>
namespace serenkai {

// NOLINTBEGIN(readability-identifier-naming)
enum class Language : std::uint8_t {
    en_US = 0, // fallback
    zh_CN,
};
// NOLINTEND(readability-identifier-naming)

} // namespace serenkai