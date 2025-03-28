cmake_minimum_required(VERSION 3.15)
project(dedupe++ VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find OpenSSL package
find_package(OpenSSL REQUIRED)

# Add source files
set(SOURCES
    src/main.cpp
    src/scanner.cpp
    src/hasher.cpp
)

# Add header files
set(HEADERS
    include/scanner.hpp
    include/hasher.hpp
    include/progress.hpp
)

# Create executable
add_executable(dedupe++ ${SOURCES} ${HEADERS})

# Include directories
target_include_directories(dedupe++ PRIVATE include)

# Link against required libraries
target_link_libraries(dedupe++ PRIVATE 
    OpenSSL::SSL
    OpenSSL::Crypto
) 