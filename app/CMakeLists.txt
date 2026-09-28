set(TARGET_NAME serenkai)

add_executable(${TARGET_NAME}
    src/main.cpp
)

target_link_libraries(${TARGET_NAME} PRIVATE 
    serenkai::core
    serenkai::warnings 
)

add_custom_target(run
    COMMAND $<TARGET_FILE:${TARGET_NAME}>
    DEPENDS ${TARGET_NAME}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} # Ensure the game can access assets from the project root
    USES_TERMINAL       
    COMMENT "Running ${TARGET_NAME}..."
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR MINGW)
    target_link_options(${TARGET_NAME} PRIVATE
        -static-libstdc++
        -static-libgcc
    )
endif()