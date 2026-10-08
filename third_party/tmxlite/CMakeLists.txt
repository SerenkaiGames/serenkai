add_library(tmxlite STATIC
    src/FreeFuncs.cpp
    src/ImageLayer.cpp
    src/LayerGroup.cpp
    src/Map.cpp
    src/Object.cpp
    src/ObjectGroup.cpp
    src/ObjectTypes.cpp
    src/Property.cpp
    src/TileLayer.cpp
    src/Tileset.cpp
    src/miniz.c
    src/detail/pugixml.cpp
)

add_library(tmxlite::tmxlite ALIAS tmxlite)

target_include_directories(tmxlite SYSTEM PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_include_directories(tmxlite PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(tmxlite PUBLIC
    TMXLITE_STATIC
)

if(MSVC)
    target_compile_definitions(tmxlite PRIVATE
        _CRT_SECURE_NO_WARNINGS
    )
endif()
