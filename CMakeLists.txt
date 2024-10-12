cmake_minimum_required(VERSION 3.29)

project(XIN
    VERSION 0.1
    LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_EXTENSIONS FALSE)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/cmake)
include(NoInSourceBuild)
#include(AddModules)

add_subdirectory(src bin)
add_subdirectory(test)
