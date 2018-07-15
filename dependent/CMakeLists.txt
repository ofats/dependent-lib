enable_testing()

project(dependent)

set(HEADER_FILES
    dependent.h
    dense_allocator.h)

set(SOURCE_FILES
    dependent.cpp
    dense_allocator.cpp)

add_library(${PROJECT_NAME} ${SOURCE_FILES} ${HEADER_FILES})

add_subdirectory(ut)
