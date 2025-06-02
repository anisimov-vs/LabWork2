// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include "core/game.h"
#include "ui/text_ui.h"
#include "util/logger.h"
#include <iostream>
#include <string>

using namespace deckstiny;

int main() {
    // Initialize logging
    auto& logger = util::Logger::getInstance();
    
    // Configure logging
    logger.setConsoleEnabled(false);
    logger.setFileEnabled(true);
    logger.setConsoleLevel(util::LogLevel::Info);  // Show Info and above on console
    logger.setFileLevel(util::LogLevel::Debug);    // Log everything to files
    logger.setLogDirectory("logs/deckstiny");
    
    LOG_INFO("system", "Deckstiny initialization started");
    LOG_INFO("main", "Initializing game...");
    
    // Create game instance
    auto game = std::make_unique<Game>();
    auto ui = std::make_shared<TextUI>();
    
    // Initialize game
    if (!game->initialize(ui)) {
        LOG_ERROR("main", "Failed to initialize game");
        return 1;
    }
    
    LOG_INFO("main", "Game initialized successfully");
    
    // Start game loop
    game->run();
    
    LOG_INFO("main", "Game shutting down");
    return 0;
} 