add_library(stb STATIC
    image.c
    stb_image.h
)

target_include_directories(stb PUBLIC ${CMAKE_CURRENT_LIST_DIR})
