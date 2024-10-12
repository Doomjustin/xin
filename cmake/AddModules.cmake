find_package(Git QUIET)

if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
    # Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMODULE_RESULT)
        if(NOT GIT_SUBMODULE_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()


option(SPDLOG_BUILD_TESTS "spdlog Build tests" OFF)
option(SPDLOG_BUILD_EXAMPLE "spdlog Build example" OFF)

option(BUILD_TESTING "Catch2 test" OFF)
option(CATCH_INSTALL_DOCS "Catch2 documents" OFF)
option(CATCH_INSTALL_EXTRAS "Catch2's extras folder" ON)

add_subdirectory(module/fmt)
add_subdirectory(module/spdlog)
add_subdirectory(module/Catch2)
add_subdirectory(module/pybind11)
