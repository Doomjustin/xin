file(GLOB UTILITY_MODULE_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cppm")
file(GLOB UTILITY_IMPLEMENT_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
file(GLOB UTILITY_TEST_SOURCE CONFIGURE_DEPENDS "${CMAKE_CURRENT_LIST_DIR}/*.test.cpp")

list(FILTER UTILITY_IMPLEMENT_SOURCE EXCLUDE REGEX "\\.test\\.cpp$")


add_library(xin.utility ${UTILITY_IMPLEMENT_SOURCE})

add_library(xin::utility ALIAS xin.utility)

target_compile_features(xin.utility PUBLIC cxx_std_23)

target_sources(xin.utility 
    PUBLIC 
        FILE_SET CXX_MODULES
    FILES
        ${UTILITY_MODULE_SOURCE}
)

target_link_libraries(xin.utility 
    PRIVATE 
        spdlog::spdlog
        magic_enum::magic_enum
)

set_property(TARGET xin.utility PROPERTY CXX_MODULE_STD ON)


if(BUILD_TESTING AND ${PROJECT_NAME}_BUILD_TESTS)
    add_executable(xin.test.utility ${UTILITY_TEST_SOURCE})

    target_link_libraries(xin.test.utility
        PRIVATE
            xin::utility
            Catch2::Catch2WithMain
    )

    target_compile_features(xin.test.utility PRIVATE cxx_std_23)
    set_property(TARGET xin.test.utility PROPERTY CXX_MODULE_STD ON)

    add_test(NAME xin.test.utility COMMAND xin.test.utility)
endif()