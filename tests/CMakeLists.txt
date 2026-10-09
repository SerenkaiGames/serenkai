find_package(Catch2 3 REQUIRED)
add_executable(serenkai_tests
    src/window_test.cpp
    src/application_test.cpp
    src/scene_test.cpp
    src/input_test.cpp
    src/widget_test.cpp
    src/gui_context_test.cpp
    src/raii_test.cpp
    src/resource_test.cpp
    src/math_test.cpp
    src/texture_test.cpp
    src/text_renderer_test.cpp
    src/font_test.cpp
    src/color_test.cpp
    src/label_test.cpp
    src/rect_test.cpp
    src/button_test.cpp
    src/widget_parser_test.cpp
    src/image_loader_test.cpp
    src/texture_manager_test.cpp
    src/image_widget_test.cpp
    src/column_layout_test.cpp
    src/script_engine_test.cpp
    src/map_loader_test.cpp
)
target_link_libraries(serenkai_tests PRIVATE
    serenkai::core
    serenkai::warnings
    Catch2::Catch2WithMain
)
target_compile_definitions(serenkai_tests PRIVATE
    SERENKAI_TEST_ASSET_DIR="${CMAKE_SOURCE_DIR}/assets"
)
include(Catch)
catch_discover_tests(serenkai_tests)