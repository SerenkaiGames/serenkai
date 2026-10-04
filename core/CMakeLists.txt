set(TARGET_NAME serenkai_core)

find_package(SDL3 CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(glaze CONFIG REQUIRED)
find_package(Freetype REQUIRED)
find_package(harfbuzz CONFIG REQUIRED)
find_package(magic_enum CONFIG REQUIRED)

add_library(
    ${TARGET_NAME} STATIC 
    src/platform.cpp
    src/application/window_manager.cpp
    include/serenkai/application/application.hpp
    include/serenkai/application/window_manager.hpp
    src/application/application.cpp
    include/serenkai/render/renderer.hpp
    src/render/renderer.cpp
    include/serenkai/scenes/scene.hpp
    include/serenkai/scenes/title_scene.hpp
    include/serenkai/scenes/scene_manager.hpp
    src/scenes/scene_manager.cpp
    include/serenkai/base/assert.hpp
    include/serenkai/base/concepts.hpp
    include/serenkai/base/type_name.hpp
    include/serenkai/base/enum_meta.hpp
    include/serenkai/base/panic.hpp
    include/serenkai/base/unreachable.hpp
    include/serenkai/scenes/game_scene.hpp
    src/scenes/game_scene.cpp
    src/scenes/title_scene.cpp
    include/serenkai/application/key.hpp
    include/serenkai/application/event.hpp
    include/serenkai/application/input.hpp
    src/application/input.cpp
    include/serenkai/gui/widget.hpp
    include/serenkai/gui/anchor.hpp
    src/gui/widget.cpp
    include/serenkai/gui/gui_context.hpp
    include/serenkai/base/glm_fmt.hpp
    src/gui/gui_context.cpp
    src/resource/asset_manager.cpp
    src/resource/directory_source.cpp
    include/serenkai/resource/asset_manager.hpp
    include/serenkai/resource/asset_source.hpp
    include/serenkai/resource/directory_source.hpp
    include/serenkai/resource/resource_location.hpp
    include/serenkai/base/raii.hpp
    include/serenkai/base/math.hpp
    include/serenkai/render/texture.hpp
    src/render/texture.cpp
    include/serenkai/render/text_renderer.hpp
    src/render/text_renderer.cpp
    include/serenkai/resource/font.hpp
    src/resource/font.cpp
    include/serenkai/gui/label.hpp
    src/gui/label.cpp
    include/serenkai/gui/color.hpp
    include/serenkai/resource/font_manager.hpp
    src/resource/font_manager.cpp
    src/gui/widget_parser.cpp
    include/serenkai/gui/widget_parser.hpp
    src/gui/rect.cpp
    include/serenkai/gui/rect.hpp
    src/gui/button.cpp
    include/serenkai/gui/button.hpp
)

add_library(serenkai::core ALIAS ${TARGET_NAME})

target_include_directories(${TARGET_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(${TARGET_NAME} PUBLIC
    SDL3::SDL3
    glm::glm
    spdlog::spdlog
    serenkai::warnings
    glaze::glaze
    Freetype::Freetype
    harfbuzz::harfbuzz
    magic_enum::magic_enum
)

if (WIN32)
    target_compile_definitions(${TARGET_NAME} PRIVATE
        NOMINMAX
        WIN32_LEAN_AND_MEAN
    )
endif()

target_compile_definitions(${TARGET_NAME} PRIVATE
    $<$<CONFIG:Debug>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_DEBUG>
    $<$<CONFIG:Release>:SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_INFO>
)

# Export executable symbols for runtime stack trace symbol resolution.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    if(APPLE)
        target_link_options(${TARGET_NAME} INTERFACE
            -Wl,-export_dynamic
        )
    elseif(UNIX)
        target_link_options(${TARGET_NAME} INTERFACE
            -rdynamic
        )
    endif()
endif()
