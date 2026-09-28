add_library(serenkai_warnings INTERFACE)

add_library(serenkai::warnings ALIAS serenkai_warnings)

if(MSVC)
    target_compile_options(serenkai_warnings INTERFACE
        /W4
        /permissive-
        /utf-8
    )
elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
    target_compile_options(serenkai_warnings INTERFACE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()