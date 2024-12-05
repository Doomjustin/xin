SET(OPENGL_CORE_VERSION_MAJOR 3)
SET(OPENGL_CORE_VERSION_MAJOR 3)

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/BuildInfo.cppm.in"
    "${CMAKE_CURRENT_LIST_DIR}/BuildInfo.cppm" @ONLY
)

add_library(${PROJECT_NAME}_BASE OBJECT)

add_library(xin::base ALIAS ${PROJECT_NAME}_BASE)

target_sources(${PROJECT_NAME}_BASE
    PUBLIC FILE_SET CXX_MODULES
    FILES
        "BuildInfo.cppm"
        "Utility.cppm"
)
