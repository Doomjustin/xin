add_library(${PROJECT_NAME} STATIC)

add_library(xin::xin ALIAS ${PROJECT_NAME})

target_sources(${PROJECT_NAME} 
    PUBLIC
        FILE_SET CXX_MODULES 
    FILES
        "database/Database.cpp"
        "database/WriteBatch.cpp"
)

target_link_libraries(${PROJECT_NAME} 
    PUBLIC
        fmt::fmt
        spdlog::spdlog
        nlohmann_json::nlohmann_json
)
