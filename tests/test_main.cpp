#include <gtest/gtest.h>
#include "util/logger.h"

int main(int argc, char **argv) {
    printf("Running test main() with testing mode enabled\n");
    
    // Set logger to testing mode
    deckstiny::util::Logger::setTestingMode(true);
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run the tests
    return RUN_ALL_TESTS();
} 