file(GLOB NET_MODULE_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cppm")
file(GLOB NET_IMPLEMENT_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB NET_TEST_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.test.cpp")

list(FILTER NET_IMPLEMENT_SOURCE EXCLUDE REGEX "\\.test\\.cpp$")


add_library(xin.net ${NET_IMPLEMENT_SOURCE})

add_library(xin::net ALIAS xin.net)

target_compile_features(xin.net PUBLIC cxx_std_23)

target_sources(xin.net 
    PUBLIC 
        FILE_SET CXX_MODULES
    FILES
        ${NET_MODULE_SOURCE}
)

target_link_libraries(xin.net 
    PRIVATE 
        xin::utility
        xin::async
)

set_property(TARGET xin.net PROPERTY CXX_MODULE_STD ON)


if(BUILD_TESTING AND ${PROJECT_NAME}_BUILD_TESTS)
    add_executable(xin.test.net ${NET_TEST_SOURCE})

    target_link_libraries(xin.test.net
        PRIVATE
            xin::net
            Catch2::Catch2WithMain
    )

    target_compile_features(xin.test.net PRIVATE cxx_std_23)
    set_property(TARGET xin.test.net PROPERTY CXX_MODULE_STD ON)

    add_test(NAME xin.test.net COMMAND xin.test.net)
endif()