#include <gtest/gtest.h>
#include "util/logger.h"
#include "ui/text_ui.h"
#include <iostream>
#include <filesystem>

int main(int argc, char **argv) {    
    ::testing::InitGoogleTest(&argc, argv);
    
    // Initialize and configure logger for testing
    deckstiny::util::Logger::init(); // Ensure initialized
    auto& logger = deckstiny::util::Logger::getInstance();
    logger.setTestingMode(true); // Disables console, sets testingMode_ flag
    logger.setFileEnabled(true);
    logger.setFileLevel(deckstiny::util::LogLevel::Debug); // Log all levels to file
    
    std::string logDirPath = "test_logs";
    try {
        if (!std::filesystem::exists(logDirPath)) {
            std::filesystem::create_directories(logDirPath);
        }
    } catch (const std::filesystem::filesystem_error& e) { }
    logger.setLogDirectory(logDirPath);
    
    logger.log(deckstiny::util::LogLevel::Info, "game", "Logger configured for testing by test_main. Log directory: " + logDirPath);
    logger.log(deckstiny::util::LogLevel::Debug, "game", "Debug level test message from test_main.");

    deckstiny::TextUI::setTestingMode(true);

    std::cout << "Running test main() with testing mode enabled" << std::endl;
    
    return RUN_ALL_TESTS();
} 