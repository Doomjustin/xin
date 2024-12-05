list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
include(CTest)
include(Catch)


add_subdirectory(base)
add_subdirectory(database)
add_subdirectory(game_engine)
