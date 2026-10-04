#pragma once

#include <string_view>
template <typename T> constexpr std::string_view type_name() {
#if defined(__clang__) || defined(__GNUC__)
    std::string_view p = __PRETTY_FUNCTION__;
    //  "std::string_view type_name() [with T = MyType]"
    auto start = p.find("T = ") + 4;
    auto end = p.find(']', start);
    return p.substr(start, end - start);
#elif defined(_MSC_VER)
    std::string_view p = __FUNCSIG__;
    //  "class std::basic_string_view<...> __cdecl type_name<MyType>(void)"
    auto start = p.find("type_name<") + 10;
    auto end = p.find('>', start);
    return p.substr(start, end - start);
#endif
}