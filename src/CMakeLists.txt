add_subdirectory(base)
add_subdirectory(database)
add_subdirectory(game_engine)

add_library(${PROJECT_NAME} INTERFACE)

add_library(xin::xin ALIAS ${PROJECT_NAME})

target_link_libraries(${PROJECT_NAME}
    INTERFACE
        xin::base
        xin::database
        xin::game_engine
)
