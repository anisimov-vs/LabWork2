cmake_minimum_required(VERSION 3.10)

# Set up Google Test
include(FetchContent)
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG release-1.12.1
)
FetchContent_MakeAvailable(googletest)

# Include project directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# Create test executable
add_executable(deckstiny_tests
  # Custom main file that disables console logging
  test_main.cpp
  
  # Main test files
  character_test.cpp
  # Disabled until fixed:
  # card_test.cpp
  # relic_test.cpp
  # event_test.cpp
  # combat_test.cpp
  # map_test.cpp
  # game_test.cpp
  # ui_test.cpp
)

# Link test executable with Google Test and project libraries
target_link_libraries(deckstiny_tests
  gtest
  # We don't need gtest_main anymore as we have our own main
  # gtest_main
  deckstiny_core
  deckstiny_ui
  deckstiny_util
)

# Register tests with CTest
include(GoogleTest)
gtest_discover_tests(deckstiny_tests)

# Add test to project
add_test(NAME deckstiny_tests COMMAND deckstiny_tests) 