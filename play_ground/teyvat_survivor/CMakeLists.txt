add_library(ts OBJECT)

target_sources(ts PUBLIC
    FILE_SET database
    TYPE CXX_MODULES
    FILES
        "Actor.cppm"
        "Animation.cppm"
        "Atlas.cppm"
        "Bullet.cppm"
        "Common.cppm"
        "Enemy.cppm"
        "Player.cppm"
        "Point.cppm"
        "Rect.cppm"
)

add_easy_x(ts)

add_executable(teyvat-survivor main.cpp)
add_easy_x(teyvat-survivor)
target_link_libraries(teyvat-survivor PRIVATE ts)

add_custom_command(
    TARGET teyvat-survivor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            resource
            $<TARGET_FILE_DIR:teyvat-survivor>/resource
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
)
