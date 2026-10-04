#pragma once

#include <string_view>
namespace serenkai {
template <typename T> constexpr std::string_view type_name() {
#if defined(__clang__)
    std::string_view p = __PRETTY_FUNCTION__;
    // "std::string_view type_name() [T = int]"
    auto start = p.find("T = ") + 4;
    auto end = p.find(']', start);
    return p.substr(start, end - start);
#elif defined(__GNUC__)
    std::string_view p = __PRETTY_FUNCTION__;
    // "constexpr std::string_view type_name() [with T = int; std::string_view =
    // std::basic_string_view<char>]"
    auto start = p.find("T = ") + 4;
    auto semicolon = p.find(';', start);
    auto bracket = p.find(']', start);
    auto end = (semicolon != std::string_view::npos && semicolon < bracket)
                   ? semicolon
                   : bracket;
    auto result = p.substr(start, end - start);

    while (!result.empty() && result.back() == ' ')
        result.remove_suffix(1);
    return result;
#elif defined(_MSC_VER)
    std::string_view p = __FUNCSIG__;
    // "class std::basic_string_view<...> __cdecl type_name<int>(void)"
    auto start = p.find("type_name<") + 10;
    auto end = p.find('>', start);
    return p.substr(start, end - start);
#else
    return "unknown";
#endif
}
} // namespace serenkai
