cmake_minimum_required(VERSION 3.29)
project(doppod)

set(CMAKE_CXX_STANDARD 20)

add_subdirectory(third_party/googletest)

add_executable(doppod main.cpp
        Shared.h
        Shared.h)

target_link_libraries(doppod PUBLIC GTest::gtest_main)

add_executable(tests
        test.cpp
        Shared.h
)

target_link_libraries(tests PRIVATE GTest::gtest_main)
