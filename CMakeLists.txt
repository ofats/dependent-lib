cmake_minimum_required(VERSION 3.7)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS_DEBUG "-O2 -Wall -Wextra -Werror -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "-Os -Wall -Wextra -Werror")

project(hz)

include(FindPkgConfig)

include_directories(./)

set(HEADER_FILE
    catch.h
    dependent.h)
set(SOURCE_FILES
    main_catch.cpp
    dependent_unittests.cpp)

add_executable(test ${SOURCE_FILES})
