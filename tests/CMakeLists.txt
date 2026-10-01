find_package(Catch2 3 REQUIRED)
add_executable(serenkai_tests
    src/window_test.cpp
    src/application_test.cpp
)
target_link_libraries(serenkai_tests PRIVATE
    serenkai::core
    serenkai::warnings
    Catch2::Catch2WithMain
)
include(Catch)
catch_discover_tests(serenkai_tests)