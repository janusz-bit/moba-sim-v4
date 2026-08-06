cmake_minimum_required(VERSION 3.25)

project(moba_sim
    VERSION 0.1.0
    DESCRIPTION "MOBA simulator"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MOBA_SIM_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(MOBA_SIM_BUILD_DOCS "Build API documentation as part of the default target" OFF)

# Shared warning flags for all targets
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow
)

if(MOBA_SIM_WARNINGS_AS_ERRORS)
    target_compile_options(project_warnings INTERFACE -Werror)
endif()

add_subdirectory(src)
add_subdirectory(docs)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
