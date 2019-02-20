cmake_minimum_required(VERSION 3.1)
project (two_plus_two_evaluator)

include(ExternalProject)

set (CMAKE_CXX_STANDARD 14)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

option(BUILD_TESTS "Build and run tests" ON)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

add_library(pokerlib SHARED pokerlib.cpp)

add_executable(generator generator.cpp)
target_link_libraries(generator pokerlib)

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(test)
endif()
