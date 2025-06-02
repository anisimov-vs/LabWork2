// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include "core/game.h"
#include "ui/graphical_ui.h"
#include "ui/text_ui.h"
#include "ui/ui_interface.h"
#include "util/logger.h"
#include <thread>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>

using namespace deckstiny;

int main(int argc, char* argv[]) {
    // Initialize logging is now handled by Game::initializeLogging()
    
    // Create game instance
    auto game = std::make_unique<Game>();
    std::shared_ptr<UIInterface> ui;

    std::vector<std::string> args(argv + 1, argv + argc);
    if (std::find(args.begin(), args.end(), "-t") != args.end()) {
        ui = std::make_shared<TextUI>();
    } else {
        ui = std::make_shared<GraphicalUI>();
    }
    
    // Initialize game
    if (!game->initialize(ui)) {
        std::cerr << "Failed to initialize game" << std::endl;
        return 1;
    }
    
    // Start game loop in a background thread
    std::thread gameThread([&](){ game->run(); });
    // Run the UI loop (blocks until shutdown)
    ui->run();
    // Wait for the game loop to finish
    if (gameThread.joinable()) {
        gameThread.join();
    }
    
    return 0;
} 