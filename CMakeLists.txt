cmake_minimum_required(VERSION 3.16)
project(nanomatch VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(NANOMATCH_BUILD_BENCH "Build Google Benchmark targets" ON)
option(NANOMATCH_BUILD_TESTS "Build unit tests" ON)

if(MSVC)
    add_compile_options(/W4 /permissive- /EHsc)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "")
        add_compile_options(-O3 -march=native)
    endif()
endif()

include(FetchContent)

if(NANOMATCH_BUILD_BENCH)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY https://github.com/google/benchmark.git
        GIT_TAG v1.8.3
    )
    FetchContent_MakeAvailable(googlebenchmark)
endif()

add_library(nanomatch_core STATIC
    src/order_book.cpp
    src/order_book_stl.cpp
    src/matcher.cpp
    src/csv_reader.cpp
)

target_include_directories(nanomatch_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

add_executable(nanomatch_cli src/main.cpp)
target_link_libraries(nanomatch_cli PRIVATE nanomatch_core)

if(NANOMATCH_BUILD_BENCH)
    add_executable(nanomatch_bench bench/benchmark.cpp)
    target_link_libraries(nanomatch_bench PRIVATE nanomatch_core benchmark::benchmark)
endif()

if(NANOMATCH_BUILD_TESTS)
    enable_testing()
    add_executable(nanomatch_tests tests/test_order_book.cpp)
    target_link_libraries(nanomatch_tests PRIVATE nanomatch_core)
    add_test(NAME order_book_tests COMMAND nanomatch_tests)
endif()
