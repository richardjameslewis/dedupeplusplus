cmake_minimum_required(VERSION 3.15)
project(dedupe++ VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find required packages
find_package(OpenSSL REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets Concurrent REQUIRED)
find_package(GTest CONFIG REQUIRED)

# Core library
add_library(dedupe_core
    core/scanner.cpp
    core/hasher.cpp
)

target_include_directories(dedupe_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/core
)

target_link_libraries(dedupe_core
    PUBLIC
        OpenSSL::SSL
        OpenSSL::Crypto
)

# Interface library
add_library(dedupe_interface
    interface/scanner_impl.cpp
)

target_include_directories(dedupe_interface
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/interface
        ${CMAKE_CURRENT_SOURCE_DIR}/core
)

target_link_libraries(dedupe_interface
    PUBLIC
        dedupe_core
)

# GUI application
add_executable(dedupe_gui
    ui/main.cpp
    ui/mainwindow.cpp
    ui/mainwindow.hpp
    ui/filesystem_model.cpp
    ui/filesystem_model.hpp
)

target_include_directories(dedupe_gui
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/ui
        ${CMAKE_CURRENT_SOURCE_DIR}/interface
)

target_link_libraries(dedupe_gui
    PRIVATE
        dedupe_interface
        Qt6::Core
        Qt6::Widgets
        Qt6::Concurrent
)

# CLI application
add_executable(dedupe_cli
    core/main.cpp
)

target_include_directories(dedupe_cli
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/core
)

target_link_libraries(dedupe_cli
    PRIVATE
        dedupe_core
)

# Tests
enable_testing()

# Create test executable
add_executable(dedupe_tests
    tests/tree_tests.cpp
)

target_link_libraries(dedupe_tests
    PRIVATE
        dedupe_core
        GTest::gtest
        GTest::gtest_main
)

# Add tests to CTest
add_test(NAME dedupe_tests COMMAND dedupe_tests) 