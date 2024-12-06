add_library(${PROJECT_NAME}_LEVELDB OBJECT
    LevelDB.cpp
    Record.cpp
    WriteBatch.cpp
)

add_library(xin::leveldb ALIAS ${PROJECT_NAME}_LEVELDB)

target_sources(${PROJECT_NAME}_LEVELDB PUBLIC
    FILE_SET database
    TYPE CXX_MODULES
    FILES
        "Coding.cppm"
        "LevelDB.cppm"
        "WriteBatch.cppm"
)

target_link_libraries(${PROJECT_NAME}_LEVELDB
    PUBLIC
        xin::base
        fmt::fmt
)
