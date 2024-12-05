add_library(${PROJECT_NAME}_DATABASE OBJECT)

add_library(xin::database ALIAS ${PROJECT_NAME}_DATABASE)

target_sources(${PROJECT_NAME}_DATABASE
    PUBLIC FILE_SET CXX_MODULES
    FILES
        "Database.cppm"
        "Coding.cppm"
)

target_link_libraries(${PROJECT_NAME}_DATABASE
    PUBLIC
        xin::base
)
