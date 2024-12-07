macro(add_easy_x target)
    target_include_directories(${target} PUBLIC ${CMAKE_SOURCE_DIR}/play_ground/EasyX/include)

    target_link_libraries(${target} PRIVATE ${CMAKE_SOURCE_DIR}/play_ground/EasyX//lib/VC2015/x64/EasyXa.lib)
endmacro()
