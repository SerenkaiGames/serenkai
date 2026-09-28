set(TARGET_NAME serenkai_core)

find_package(SDL3 CONFIG REQUIRED)

add_library(
    ${TARGET_NAME} STATIC 
    src/platform.cpp
)

add_library(serenkai::core ALIAS ${TARGET_NAME})

target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${TARGET_NAME} PRIVATE
    SDL3::SDL3
    serenkai::warnings
)
