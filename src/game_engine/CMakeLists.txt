add_library(${PROJECT_NAME}_GAME_ENGINE OBJECT)

add_library(xin::game_engine ALIAS ${PROJECT_NAME}_GAME_ENGINE)

target_sources(${PROJECT_NAME}_GAME_ENGINE
    PUBLIC FILE_SET CXX_MODULES
    FILES
        "GameEngine.cppm"
)
