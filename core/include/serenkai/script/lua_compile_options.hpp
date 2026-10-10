#pragma once
#include <luacode.h>

namespace serenkai::detail {
inline lua_CompileOptions get_lua_options() {

    lua_CompileOptions options{};
#ifdef NDEBUG
    // Release
    options.optimizationLevel = 2;
    options.debugLevel = 0;
#else
    // Debug
    options.optimizationLevel = 1;
    options.debugLevel = 2;
#endif
    options.typeInfoLevel = 1;

    return options;
}
} // namespace serenkai::detail