add_subdirectory(base)
add_subdirectory(leveldb)
add_subdirectory(game_engine)
add_subdirectory(stb)

add_library(${PROJECT_NAME} INTERFACE)

add_library(xin::xin ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        xin::base
        xin::leveldb
        xin::game_engine
)
