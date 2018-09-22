project(dependent_benchmarks)

set(MEMORY_BENCHMARKS_SOURCE_FILES
    read_huge_map_memory_benchmark.cpp
)

add_executable(${PROJECT_NAME}_memory ${MEMORY_BENCHMARKS_SOURCE_FILES})
