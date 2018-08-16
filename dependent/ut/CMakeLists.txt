enable_testing()

project(dependent_ut)

set(SOURCE_FILES
    dependent_ut.cpp
    future_std_stubs_ut.cpp)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} dependent catch)

add_test(NAME ${PROJECT_NAME} COMMAND ${PROJECT_NAME})
