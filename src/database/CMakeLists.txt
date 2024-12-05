add_library(${PROJECT_NAME}_DATABASE
    OBJECT
        Coding.cpp
        Coding.h
)

add_library(xin::database ALIAS ${PROJECT_NAME}_DATABASE)

target_include_directories(${PROJECT_NAME}_DATABASE PUBLIC ${CMAKE_CURRENT_LIST_DIR})
