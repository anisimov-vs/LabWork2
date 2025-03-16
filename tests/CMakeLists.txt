set(TEST_SOURCES
    test.cpp
)

# Find GTest
find_package(GTest REQUIRED)
include_directories(${GTEST_INCLUDE_DIRS})

# Create test executable
add_executable(tests ${TEST_SOURCES})

# Link against GTest libraries - use imported targets for better cross-platform compatibility
target_link_libraries(tests GTest::GTest GTest::Main)

# Add pthread if needed
if(UNIX AND NOT APPLE)
  target_link_libraries(tests pthread)
endif()

# Register the test
add_test(NAME AllTests COMMAND tests) 