cmake_minimum_required(VERSION 3.14)
project(asun_c C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Release by default
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

# Library
add_library(asun STATIC src/asun.c)
target_include_directories(asun PUBLIC include)

# Examples
add_executable(basic examples/basic.c)
target_link_libraries(basic asun m)

add_executable(complex_example examples/complex.c)
target_link_libraries(complex_example asun m)

add_executable(bench examples/bench.c)
target_link_libraries(bench asun m)

add_executable(cross_compat examples/cross_compat.c)
target_link_libraries(cross_compat asun m)

# Tests
add_executable(asun_test tests/asun_test.c)
target_link_libraries(asun_test asun m)

add_executable(cross_compat_test tests/cross_compat_test.c)
target_link_libraries(cross_compat_test asun m)

enable_testing()
add_test(NAME asun_test COMMAND asun_test)
add_test(NAME cross_compat_test COMMAND cross_compat_test)
