set(TARGET_NAME serenkai_core)

find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

add_library(
    ${TARGET_NAME} STATIC 
    src/platform.cpp
    src/application/window_manager.cpp
    include/serenkai/application/application.hpp
    include/serenkai/application/window_manager.hpp
    src/application/application.cpp
    include/serenkai/render/renderer.hpp
    src/render/renderer.cpp
)

add_library(serenkai::core ALIAS ${TARGET_NAME})

target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${TARGET_NAME} PRIVATE
    SDL3::SDL3
    glm::glm
    spdlog::spdlog
    serenkai::warnings
)
