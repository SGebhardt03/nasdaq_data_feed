cmake_minimum_required(VERSION 3.20)
project(feedhandler LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)        # -std=c++20, not gnu++20
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) # for clangd

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()

option(FEED_NATIVE_ARCH "Build release with -march=native" ON)

add_library(feed_flags INTERFACE)
target_compile_options(feed_flags INTERFACE
        -Wall -Wextra -Wpedantic -Wconversion
        $<$<CONFIG:Debug>:-O0;-g>
        $<$<CONFIG:Release>:-O3;-fno-omit-frame-pointer>)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-march=native FEED_HAS_MARCH_NATIVE)
if(FEED_NATIVE_ARCH AND FEED_HAS_MARCH_NATIVE)
    target_compile_options(feed_flags INTERFACE $<$<CONFIG:Release>:-march=native>)
endif()

find_package(ZLIB REQUIRED)

add_library(feed_core INTERFACE)
target_include_directories(feed_core INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/itch)  # stream_reader.cpp: #include "endian.hpp"
target_link_libraries(feed_core INTERFACE feed_flags ZLIB::ZLIB)

add_executable(itch_stat src/main.cpp)
target_link_libraries(itch_stat PRIVATE feed_core)

enable_testing()

find_package(GTest REQUIRED)
include(GoogleTest)

add_executable(unit_tests
        tests/test_book.cpp)
target_link_libraries(unit_tests PRIVATE feed_core GTest::gtest GTest::gtest_main)
gtest_discover_tests(unit_tests)

add_executable(acceptance_stream_reader_100mb
        tests/acceptance_stream_reader_100mb.cpp)
target_link_libraries(acceptance_stream_reader_100mb PRIVATE feed_core)
target_include_directories(acceptance_stream_reader_100mb PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src)  # #include "stream_reader.cpp"

add_test(NAME acceptance_stream_reader_100mb
        COMMAND acceptance_stream_reader_100mb
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/itch_100mb.gz
        ${CMAKE_CURRENT_SOURCE_DIR}/tests/fixtures/itch_100mb.expected)
add_executable(feed_tests tests/test_message_parser.cpp)
