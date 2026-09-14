cmake_minimum_required(VERSION 3.20)
project(SentinelRT VERSION 0.0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

add_library(rt_project_warnings INTERFACE)
target_compile_options(rt_project_warnings INTERFACE -Wall -Wextra -Wpedantic)

enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.15.2
)
FetchContent_MakeAvailable(googletest)

add_subdirectory(src/common)
add_subdirectory(src/apps)
add_subdirectory(tests)
