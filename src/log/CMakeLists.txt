add_library(XIN_LOG OBJECT)

add_library(xin::log ALIAS XIN_LOG)

target_sources(XIN_LOG
  PUBLIC
    FILE_SET CXX_MODULES 
    FILES
        Log.cpp
)

target_link_libraries(XIN_LOG PRIVATE 
    fmt::fmt
    spdlog::spdlog
)