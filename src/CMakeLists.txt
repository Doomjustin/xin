add_subdirectory(utility)
add_subdirectory(async)
add_subdirectory(net)


add_library(xin)

add_library(xin::xin ALIAS xin)

target_compile_features(xin PUBLIC cxx_std_23)

target_sources(xin
    PUBLIC 
        FILE_SET CXX_MODULES
    FILES
        xin.cppm
)

target_link_libraries(xin 
    PRIVATE 
        xin::utility
        xin::async
        xin::net
)

set_property(TARGET xin PROPERTY CXX_MODULE_STD ON)