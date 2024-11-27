add_library(glad STATIC
    include/glad/glad.h
    include/KHR/khrplatform.h
    src/glad.c
)

target_include_directories(glad PUBLIC include)

add_library(glad::glad ALIAS glad)

set(OPENGL_VERSION_MAJOR 3)
set(OPENGL_VERSION_MINOR 3)
