add_library(stb STATIC
    stb_vorbis.cpp
    stb_image.cpp
    stb_image_write.cpp
)

target_include_directories(stb PUBLIC
    ${CMAKE_SOURCE_DIR}/third_party/stb
)
