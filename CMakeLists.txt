cmake_minimum_required(VERSION 3.21)

project(Serenkai LANGUAGES C CXX)

set(TARGET_NAME serenkai)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

find_package(SDL3 REQUIRED)

add_executable(${TARGET_NAME}
    src/main.cpp
)

add_custom_target(run
    COMMAND $<TARGET_FILE:${TARGET_NAME}>
    DEPENDS ${TARGET_NAME}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} # Ensure the game can access assets from the project root
    USES_TERMINAL       
    COMMENT "Running ${TARGET_NAME}..."
)

target_link_libraries(${TARGET_NAME}
    PRIVATE
        SDL3::SDL3
)

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(${TARGET_NAME}
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
    )
elseif(MSVC)
    target_compile_options(${TARGET_NAME}
        PRIVATE
            /W4
    )
endif()

if (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_link_options(${TARGET_NAME} PRIVATE
        -static-libstdc++
        -static-libgcc
    )
endif()

if(WIN32)
    add_custom_command(
        TARGET ${PROJECT_NAME}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:${PROJECT_NAME}>
            $<TARGET_FILE_DIR:${PROJECT_NAME}>
        COMMAND_EXPAND_LISTS
    )
endif()