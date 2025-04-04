cmake_minimum_required(VERSION 3.10)
project(Deckstiny VERSION 0.1.0 LANGUAGES CXX)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Set compiler flags
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror -Wpedantic")
endif()

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Add source files
set(SOURCES
    src/main.cpp
    src/core/Game.cpp
)

# Create main executable
add_executable(deckstiny ${SOURCES})

# Add Google Test
enable_testing()
add_subdirectory(tests) 