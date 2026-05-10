file(GLOB_RECURSE ASYNC_MODULE_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cppm")
file(GLOB_RECURSE ASYNC_IMPLEMENT_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB_RECURSE ASYNC_TEST_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.test.cpp")

list(FILTER ASYNC_IMPLEMENT_SOURCE EXCLUDE REGEX "\\.test\\.cpp$")


add_library(xin.async ${ASYNC_IMPLEMENT_SOURCE})

add_library(xin::async ALIAS xin.async)

target_compile_features(xin.async PUBLIC cxx_std_23)

target_sources(xin.async 
    PUBLIC 
        FILE_SET CXX_MODULES
    FILES
        ${ASYNC_MODULE_SOURCE}
)

target_link_libraries(xin.async 
    PRIVATE 
        xin::utility
        PkgConfig::liburing
)

set_property(TARGET xin.async PROPERTY CXX_MODULE_STD ON)


if(BUILD_TESTING AND ${PROJECT_NAME}_BUILD_TESTS)
    add_executable(xin.test.async ${ASYNC_TEST_SOURCE})

    target_link_libraries(xin.test.async
        PRIVATE
            xin::async
            Catch2::Catch2WithMain
    )

    target_compile_features(xin.test.async PRIVATE cxx_std_23)
    set_property(TARGET xin.test.async PROPERTY CXX_MODULE_STD ON)

    add_test(NAME xin.test.async COMMAND xin.test.async)
endif()