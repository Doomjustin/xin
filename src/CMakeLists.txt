add_subdirectory(database)

add_library(${PROJECT_NAME} STATIC)

add_library(xin::xin ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME} 
    PUBLIC
        FILE_SET CXX_MODULES 
    FILES
        "Database.cppm"
        "Utility.cppm"
)

target_link_libraries(${PROJECT_NAME} 
    PUBLIC
        fmt::fmt
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        xin::database
)
