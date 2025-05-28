cmake_minimum_required(VERSION 3.15)
project(PerformanceTests VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(GTest REQUIRED)

add_executable(performance_tests
    performance_test.cpp
)

target_link_libraries(performance_tests
    PRIVATE
    GTest::GTest
    GTest::Main
)

add_test(NAME performance_tests COMMAND performance_tests) 