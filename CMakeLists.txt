cmake_minimum_required(VERSION 3.10)
project(Deckstiny VERSION 1.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Option to build tests
option(BUILD_TESTS "Build the tests" OFF)

# Include directories
include_directories(include)

# Find required packages
find_package(nlohmann_json 3.9.1 QUIET)
if(NOT nlohmann_json_FOUND)
    message(STATUS "nlohmann_json not found, using bundled version")
    add_subdirectory(external/json)
endif()

# Add subdirectories for components first
add_subdirectory(src/util)

# Group source files by component
file(GLOB_RECURSE CORE_SOURCES "src/core/*.cpp")
file(GLOB_RECURSE UI_SOURCES "src/ui/*.cpp")
file(GLOB_RECURSE MAIN_SOURCES "src/main.cpp")

# Create libraries for components (useful for testing)
add_library(deckstiny_core STATIC ${CORE_SOURCES})
add_library(deckstiny_ui STATIC ${UI_SOURCES})
target_link_libraries(deckstiny_core PUBLIC nlohmann_json::nlohmann_json PUBLIC deckstiny_util)
target_link_libraries(deckstiny_ui PUBLIC deckstiny_core)

# Add executable
add_executable(deckstiny ${MAIN_SOURCES})
target_link_libraries(deckstiny 
    PRIVATE deckstiny_core
    PRIVATE deckstiny_ui
    PRIVATE deckstiny_util
)

# Copy data files to build directory
file(COPY ${CMAKE_SOURCE_DIR}/data DESTINATION ${CMAKE_BINARY_DIR})

# Additional compiler warnings
if(MSVC)
    target_compile_options(deckstiny PRIVATE /W4)
    target_compile_options(deckstiny_core PRIVATE /W4)
    target_compile_options(deckstiny_ui PRIVATE /W4)
else()
    target_compile_options(deckstiny PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(deckstiny_core PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(deckstiny_ui PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Add tests if enabled
if(BUILD_TESTS)
    message(STATUS "Building tests")
    enable_testing()
    add_subdirectory(tests)
endif() 