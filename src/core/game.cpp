#include "core/game.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/combat.h"
#include "core/card.h"
#include "core/relic.h"
#include "core/map.h"
#include "core/event.h"
#include "ui/ui_interface.h"
#include "util/logger.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <random>
#include <chrono>
#include <algorithm>
#include <filesystem>
#include <nlohmann/json.hpp>
#include <thread>

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace deckstiny {

Game::Game() 
    : state_(GameState::MAIN_MENU) {
    // Initialize logging system
    //initializeLogging();
}

Game::~Game() {
    // Clean up any resources
    util::Logger::getInstance().log(util::LogLevel::Info, "game", "Game shutting down");
}

void Game::initializeLogging() {
    // Initialize logger
    util::Logger::init();
    auto& logger = util::Logger::getInstance();
    
    // Configure logging
    logger.setConsoleEnabled(true);
    logger.setFileEnabled(true);
    logger.setConsoleLevel(util::LogLevel::Info);  // Show Info and above on console
    logger.setFileLevel(util::LogLevel::Debug);    // Log everything to files
    logger.setLogDirectory("logs/slay");
    
    LOG_INFO("system", "Deckstiny initialization started");
}

bool Game::initialize(std::shared_ptr<UIInterface> uiInterface) {
    ui_ = uiInterface;
    
    if (!ui_->initialize(this)) {
        LOG_ERROR("system", "Failed to initialize UI");
        return false;
    }
    
    LOG_DEBUG("system", "UI initialized successfully");
    
    // Set up UI input handling
    ui_->setInputCallback([this](const std::string& input) {
        return processInput(input);
    });
    
    LOG_DEBUG("system", "Input callback registered");
    
    // Initialize input handlers
    initializeInputHandlers();
    LOG_DEBUG("system", "Input handlers initialized");
    
    // Load game data
    if (!loadGameData()) {
        LOG_ERROR("system", "Failed to load game data");
        return false;
    }
    
    LOG_INFO("system", "Game initialization completed successfully");
    return true;
}

void Game::run() {
    LOG_INFO("game", "Starting game loop");
    running_ = true;
    
    // Show main menu
    setState(GameState::MAIN_MENU);
    
    LOG_INFO("game", "Game loop started");
    
    while (running_) {
        // Check if any threads need to be joined
        // Game loop is handled by UI callbacks
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    LOG_INFO("game", "Game loop ended");
}

void Game::shutdown() {
    LOG_INFO("game", "Shutting down game");
    running_ = false;
    LOG_INFO("game", "Game shutdown complete");
}

void Game::setState(GameState state) {
    LOG_DEBUG("game", "Changing state from " + std::to_string(static_cast<int>(state_)) + 
             " to " + std::to_string(static_cast<int>(state)));
    
    // Store old state for transition logic (unused for now but available for future features)
    // GameState oldState = state_;
    
    // Update the state
    state_ = state;
    
    // Update UI when state changes
    switch (state_) {
        case GameState::MAIN_MENU: {
            LOG_DEBUG("game", "Showing main menu");
            ui_->showMainMenu();
            break;
        }
            
        case GameState::CHARACTER_SELECT: {
            // Get available character classes
            LOG_DEBUG("game", "Showing character selection menu");
            ui_->showCharacterSelection({"IRONCLAD", "SILENT"});
            LOG_DEBUG("game", "Character selection menu shown");
            break;
        }
            
        case GameState::MAP: {
            LOG_DEBUG("game", "Showing map");
            if (map_) {
                // Get the current room ID
                int currentRoomId = -1;
                if (map_->getCurrentRoom()) {
                    currentRoomId = map_->getCurrentRoom()->id;
                }
                ui_->showMap(currentRoomId, map_->getAvailableRooms(), map_->getAllRooms());
                LOG_DEBUG("game", "Map shown");
            } else {
                LOG_ERROR("game", "Cannot show map: map is null");
            }
            break;
        }
            
        case GameState::COMBAT: {
            LOG_DEBUG("game", "Showing combat");
            if (currentCombat_) {
                ui_->showCombat(currentCombat_.get());
                LOG_DEBUG("game", "Combat shown");
            } else {
                LOG_ERROR("game", "Cannot show combat: combat is null");
            }
            break;
        }
            
        case GameState::EVENT: {
            LOG_DEBUG("game", "Showing event");
            if (currentEvent_) {
                ui_->showEvent(currentEvent_.get(), player_.get());
                LOG_DEBUG("game", "Event shown");
            } else {
                LOG_ERROR("game", "Cannot show event: event is null");
            }
            break;
        }
            
        case GameState::GAME_OVER: {
            LOG_DEBUG("game", "Showing game over");
            
            // CRITICAL FIX: First clean up any remaining state from combat
            // We want to make sure all combat-related objects are properly reset
            if (currentCombat_) {
                LOG_INFO("game", "Cleaning up combat state before game over");
                currentCombat_.reset();
            }
            transitioningFromCombat_ = false;
            
            // Calculate final score and log it
            int finalScore = calculateScore();
            LOG_INFO("game", "Game over - Final score: " + std::to_string(finalScore));
            
            // Show the game over screen with the appropriate victory state
            // Victory is true if we defeated the final boss
            ui_->showGameOver(map_ && map_->isBossDefeated(), finalScore);
            
            LOG_DEBUG("game", "Game over shown, awaiting transition to main menu");
            
            // CRITICAL FIX: Forcibly schedule a transition back to main menu
            // This uses a detached thread to execute the transition after a short delay
            // So even if the normal input handling is stuck, this will force the transition
            LOG_INFO("game", "Scheduling automatic transition to main menu in 5 seconds");
            std::thread([this]() {
                // Wait for 5 seconds to give UI time to display game over
                std::this_thread::sleep_for(std::chrono::seconds(5));
                
                // Execute the GAME_OVER input handler directly
                LOG_INFO("game", "Auto-executing game over handler to return to main menu");
                
                // Clean up any remaining state
                LOG_INFO("game", "Auto-cleaning game state after game over");
                currentCombat_.reset();
                transitioningFromCombat_ = false;
                currentEvent_.reset();
                
                // Reset the game state for a new run
                LOG_INFO("game", "Auto-resetting map and player for new game");
                map_.reset();
                player_.reset();
                
                // Return to main menu
                LOG_INFO("game", "Auto-transitioning to main menu after game over");
                setState(GameState::MAIN_MENU);
                
                LOG_INFO("game", "Auto-returned to main menu after game over");
            }).detach();  // Detach the thread so it runs independently
            
            break;
        }
            
        case GameState::REWARD: {
            LOG_DEBUG("game", "Showing combat rewards");
            // Note: The rewards UI is already shown in endCombat before transitioning to this state
            // We don't need to do anything here except log the state change
            break;
        }
            
        default: {
            LOG_ERROR("game", "Unhandled game state in setState: " + std::to_string(static_cast<int>(state)));
            break;
        }
    }
}

bool Game::createPlayer(const std::string& className, const std::string& playerName) {
    try {
        LOG_INFO("game", "Creating player with class: " + className);
        
        // Convert className to lowercase for file lookup
        std::string classNameLower = className;
        std::transform(classNameLower.begin(), classNameLower.end(), classNameLower.begin(), 
                      [](unsigned char c) { return std::tolower(c); });
        
        LOG_DEBUG("game", "Looking for character file: " + classNameLower + ".json");
        
        // Determine player class enum
        PlayerClass playerClass = PlayerClass::IRONCLAD; // Default
        if (classNameLower == "ironclad") {
            playerClass = PlayerClass::IRONCLAD;
        } else if (classNameLower == "silent") {
            playerClass = PlayerClass::SILENT;
        } else if (classNameLower == "defect") {
            playerClass = PlayerClass::DEFECT;
        } else if (classNameLower == "watcher") {
            playerClass = PlayerClass::WATCHER;
        }
        
        // Load character data
        std::string filePath = "data/characters/" + classNameLower + ".json";
        std::ifstream file(filePath);
        if (!file.is_open()) {
            LOG_ERROR("game", "Failed to open character file: " + filePath);
            return false;
        }
        
        json characterData;
        file >> characterData;
        file.close();
        
        // Extract basic info
        std::string name = playerName.empty() ? characterData.value("name", classNameLower) : playerName;
        int maxHealth = characterData.value("max_health", 80);
        int baseEnergy = characterData.value("base_energy", 3);
        int initialHandSize = characterData.value("initial_hand_size", 5); // Default to 5 if not specified
        
        // Create player
        LOG_INFO("game", "Creating player: " + name + " (class: " + classNameLower + 
                ", health: " + std::to_string(maxHealth) + ", energy: " + std::to_string(baseEnergy) + ")");
        player_ = std::make_unique<Player>(classNameLower, name, playerClass, maxHealth, baseEnergy, initialHandSize);
        
        // Log the initial hand size for debugging
        LOG_INFO("game", "Player created with initial hand size: " + std::to_string(initialHandSize));
        
        // Add starting deck
        int cardsAdded = 0;
        if (characterData.contains("starting_deck") && characterData["starting_deck"].is_array()) {
            LOG_INFO("game", "Adding starting deck with " + std::to_string(characterData["starting_deck"].size()) + " cards");
            
            // Count how many cards are supposed to be in the starter deck
            int expectedCardCount = characterData["starting_deck"].size();
            
            for (const auto& cardId : characterData["starting_deck"]) {
                std::string cardIdStr = cardId.get<std::string>();
                LOG_INFO("game", "Trying to load card: " + cardIdStr);
                auto card = loadCard(cardIdStr);
                if (card) {
                    player_->addCard(card);
                    cardsAdded++;
                    LOG_INFO("game", "Added card to deck: " + cardIdStr + " (" + std::to_string(cardsAdded) + 
                            " of " + std::to_string(expectedCardCount) + ")");
                } else {
                    LOG_ERROR("game", "Failed to load card: " + cardIdStr);
                }
            }
            
            // Verify the exact count
            LOG_INFO("game", "Successfully added " + std::to_string(cardsAdded) + 
                    " cards to starting deck. Expected: " + std::to_string(expectedCardCount) + 
                    ", Draw pile size: " + std::to_string(player_->getDrawPile().size()));
            
            // If we didn't add all cards, something went wrong
            if (cardsAdded != expectedCardCount) {
                LOG_ERROR("game", "MISMATCH: Added " + std::to_string(cardsAdded) + 
                         " cards but expected " + std::to_string(expectedCardCount));
            }
            
            // Double-check total deck size 
            int totalDeckSize = player_->getDrawPile().size() + player_->getDiscardPile().size() + 
                               player_->getHand().size() + player_->getExhaustPile().size();
            LOG_INFO("game", "Total deck size: " + std::to_string(totalDeckSize));
            
        } else {
            LOG_WARNING("game", "No starting deck found in character data");
        }
        
        // Add starting relics
        if (characterData.contains("starting_relics") && characterData["starting_relics"].is_array()) {
            LOG_INFO("game", "Adding starting relics");
            for (const auto& relicId : characterData["starting_relics"]) {
                LOG_DEBUG("game", "Trying to load relic: " + relicId.get<std::string>());
                auto relic = loadRelic(relicId);
                if (relic) {
                    player_->addRelic(relic);
                    LOG_DEBUG("game", "Added relic: " + relicId.get<std::string>());
                } else {
                    LOG_ERROR("game", "Failed to load relic: " + relicId.get<std::string>());
                }
            }
        } else {
            LOG_WARNING("game", "No starting relics found in character data");
        }
        
        LOG_INFO("game", "Player created successfully");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("game", "Error creating player: " + std::string(e.what()));
        return false;
    }
}

Combat* Game::getCurrentCombat() const {
    return currentCombat_.get();
}

bool Game::startCombat(const std::vector<std::string>& enemies) {
    if (!player_) {
        return false;
    }
    
    // Create combat
    currentCombat_ = std::make_unique<Combat>(player_.get());
    
    // Set the game instance in the combat
    currentCombat_->setGame(this);
    
    // Add enemies
    for (const auto& enemyId : enemies) {
        auto enemy = loadEnemy(enemyId);
        if (enemy) {
            currentCombat_->addEnemy(enemy);
        }
    }
    
    // Set up combat
    player_->beginCombat();
    currentCombat_->start();
    
    // Update state
    setState(GameState::COMBAT);
    
    return true;
}

void Game::endCombat(bool victorious) {
    if (!currentCombat_) {
        LOG_ERROR("game", "No active combat to end");
        return;
    }
    
    LOG_INFO("game", "Ending combat. Victory: " + std::string(victorious ? "true" : "false"));
    
    // Mark that we're transitioning to prevent double transitions
    transitioningFromCombat_ = true;
    
    try {
        // Clean up player combat state first, but don't crash if it fails
        try {
            player_->endCombat();
        } catch (const std::exception& e) {
            LOG_ERROR("game", "Exception in player->endCombat: " + std::string(e.what()));
            // Continue anyway - we need to clean up the rest of the combat state
        }
        
        // End combat instance - mark it as over
        if (currentCombat_) {
            try {
                currentCombat_->end(victorious);
            } catch (const std::exception& e) {
                LOG_ERROR("game", "Exception in combat->end: " + std::string(e.what()));
                // Continue anyway
            }
        }
        
        if (victorious) {
            // Calculate rewards
            int goldReward = 0;
            if (currentCombat_) {
                for (const auto& enemy : currentCombat_->getEnemies()) {
                    if (enemy) {
                        try {
                            goldReward += enemy->rollGoldReward();
                        } catch (const std::exception& e) {
                            LOG_ERROR("game", "Exception rolling gold reward: " + std::string(e.what()));
                            // Continue with the gold we've calculated so far
                        }
                    }
                }
            }
            
            LOG_INFO("game", "Combat victory! Gold reward: " + std::to_string(goldReward));
            
            // Give the gold reward to the player
            player_->addGold(goldReward);
            
            // Mark current room as visited if on map
            if (map_) {
                try {
                    map_->markCurrentRoomVisited();
                    
                    // Check if this was a boss room
                    const Room* currentRoom = map_->getCurrentRoom();
                    if (currentRoom && currentRoom->type == RoomType::BOSS) {
                        LOG_INFO("game", "Boss defeated! Generating next act");
                        
                        // Mark boss as defeated
                        map_->markBossDefeated();
                        
                        // Generate new map for next act
                        int nextAct = map_->getAct() + 1;
                        if (generateMap(nextAct)) {
                            LOG_INFO("game", "Generated new map for act " + std::to_string(nextAct));
                        } else {
                            LOG_ERROR("game", "Failed to generate new map for act " + std::to_string(nextAct));
                            // Safe cleanup before exiting
                            currentCombat_.reset();
                            transitioningFromCombat_ = false;
                            setState(GameState::GAME_OVER);
                            return;
                        }
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR("game", "Exception processing map after combat: " + std::string(e.what()));
                    // Continue anyway - just show the rewards screen
                }
            }
            
            // Always show the rewards screen, even if no gold was earned
            LOG_INFO("game", "Showing rewards screen. Gold earned: " + std::to_string(goldReward));
            
            // Show the rewards screen
            if (ui_) {
                try {
                    ui_->showRewards(goldReward, {}, {});
                } catch (const std::exception& e) {
                    LOG_ERROR("game", "Exception showing rewards: " + std::string(e.what()));
                    // Continue anyway
                }
            }
            
            // Important: Clean up combat state BEFORE changing game state
            // This prevents issues with the combat state being accessed after it's destroyed
            LOG_INFO("game", "Clearing combat state");
            currentCombat_.reset();
            
            // Reset transition flag
            transitioningFromCombat_ = false;
            
            // Return to map
            LOG_INFO("game", "Transitioning to map view");
            setState(GameState::MAP);
        } else {
            // Player defeated - handle game over
            LOG_INFO("game", "Player defeated! Transitioning to game over screen");
            
            // Calculate final score
            int finalScore = calculateScore();
            LOG_INFO("game", "Final score: " + std::to_string(finalScore));
            
            // Clean up combat state
            currentCombat_.reset();
            
            // Reset transition flag
            transitioningFromCombat_ = false;
            
            // Transition to game over state
            setState(GameState::GAME_OVER);
            LOG_INFO("game", "Transitioned to game over state");
        }
    } catch (const std::exception& e) {
        // Handle any exceptions to prevent crashes
        LOG_ERROR("game", "Exception in endCombat: " + std::string(e.what()));
        
        // Ensure we clean up even if an exception occurs
        currentCombat_.reset();
        transitioningFromCombat_ = false;
        
        // Go to a safe state
        setState(GameState::GAME_OVER);
        LOG_INFO("game", "Forced transition to game over state due to exception");
    }
}

GameMap* Game::getMap() const {
    return map_.get();
}

bool Game::generateMap(int act) {
    map_ = std::make_unique<GameMap>();
    return map_->generate(act);
}

std::shared_ptr<Card> Game::loadCard(const std::string& id) {
    // Check if already loaded
    auto it = cardTemplates_.find(id);
    if (it != cardTemplates_.end()) {
        return it->second->cloneCard();
    }
    
    try {
        // Load from file
        fs::path cardPath = "data/cards/" + id + ".json";
        std::ifstream file(cardPath);
        if (!file.is_open()) {
            std::cerr << "Could not open card file: " << cardPath << std::endl;
            return nullptr;
        }
        
        json cardData;
        file >> cardData;
        
        // Create card
        auto card = std::make_shared<Card>();
        if (!card->loadFromJson(cardData)) {
            std::cerr << "Failed to load card: " << id << std::endl;
            return nullptr;
        }
        
        // Cache template
        cardTemplates_[id] = card;
        
        // Return a clone
        return card->cloneCard();
    } catch (const std::exception& e) {
        std::cerr << "Error loading card: " << e.what() << std::endl;
        return nullptr;
    }
}

bool Game::loadAllCards() {
    try {
        for (const auto& entry : fs::directory_iterator("data/cards")) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string id = entry.path().stem().string();
                if (!loadCard(id)) {
                    std::cerr << "Failed to load card: " << id << std::endl;
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading cards: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<Enemy> Game::loadEnemy(const std::string& id) {
    // Check if already loaded
    auto it = enemyTemplates_.find(id);
    if (it != enemyTemplates_.end()) {
        return it->second->cloneEnemy();
    }
    
    try {
        // Load from file
        fs::path enemyPath = "data/enemies/" + id + ".json";
        std::ifstream file(enemyPath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open enemy file: " << enemyPath << std::endl;
            return nullptr;
        }
        
        json enemyData;
        try {
            file >> enemyData;
        } catch (const json::parse_error& e) {
            std::cerr << "Error: JSON parse error in file " << enemyPath << ": " << e.what() << std::endl;
            return nullptr;
        }
        
        // Create enemy
        auto enemy = std::make_shared<Enemy>();
        if (!enemy->loadFromJson(enemyData)) {
            std::cerr << "Error: Failed to load enemy: " << id << std::endl;
            return nullptr;
        }
        
        // Cache template
        enemyTemplates_[id] = enemy;
        
        // Return a clone
        return enemy->cloneEnemy();
    } catch (const std::exception& e) {
        std::cerr << "Error loading enemy: " << e.what() << std::endl;
        return nullptr;
    }
}

bool Game::loadAllEnemies() {
    try {
        for (const auto& entry : fs::directory_iterator("data/enemies")) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string id = entry.path().stem().string();
                if (!loadEnemy(id)) {
                    std::cerr << "Failed to load enemy: " << id << std::endl;
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading enemies: " << e.what() << std::endl;
        return false;
    }
}

std::shared_ptr<Relic> Game::loadRelic(const std::string& id) {
    // Check if already loaded
    auto it = relicTemplates_.find(id);
    if (it != relicTemplates_.end()) {
        return it->second->cloneRelic();
    }
    
    try {
        // Load from file
        fs::path relicPath = "data/relics/" + id + ".json";
        std::ifstream file(relicPath);
        if (!file.is_open()) {
            std::cerr << "Could not open relic file: " << relicPath << std::endl;
            return nullptr;
        }
        
        json relicData;
        file >> relicData;
        
        // Create relic
        auto relic = std::make_shared<Relic>();
        if (!relic->loadFromJson(relicData)) {
            std::cerr << "Failed to load relic: " << id << std::endl;
            return nullptr;
        }
        
        // Cache template
        relicTemplates_[id] = relic;
        
        // Return a clone
        return relic->cloneRelic();
    } catch (const std::exception& e) {
        std::cerr << "Error loading relic: " << e.what() << std::endl;
        return nullptr;
    }
}

bool Game::loadAllRelics() {
    try {
        for (const auto& entry : fs::directory_iterator("data/relics")) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string id = entry.path().stem().string();
                if (!loadRelic(id)) {
                    std::cerr << "Failed to load relic: " << id << std::endl;
                }
            }
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading relics: " << e.what() << std::endl;
        return false;
    }
}

UIInterface* Game::getUI() const {
    return ui_.get();
}

bool Game::processInput(const std::string& input) {
    if (!running_) {
        LOG_DEBUG("game", "Game is not running, ignoring input: " + input);
        return false;
    }
    
    LOG_DEBUG("game", "Processing input: '" + input + "' in state " + std::to_string(static_cast<int>(state_)));
    
    // Special handling for game over state
    if (state_ == GameState::GAME_OVER) {
        LOG_INFO("game", "Processing input in GAME_OVER state: '" + input + "'");
        
        // Any input in game over state should return to main menu
        auto it = inputHandlers_.find(state_);
        if (it != inputHandlers_.end()) {
            return it->second("continue");
        }
    }
    
    // Handle global inputs first
    if (input == "quit" || input == "exit") {
        LOG_INFO("game", "User requested quit, shutting down");
        shutdown();
        return true;
    }
    
    // Find the input handler for the current state
    auto it = inputHandlers_.find(state_);
    if (it != inputHandlers_.end()) {
        // Call the handler
        return it->second(input);
    }
    
    LOG_WARNING("game", "No input handler for state " + std::to_string(static_cast<int>(state_)));
    return false;
}

void Game::initializeInputHandlers() {
    // Main menu input handler
    inputHandlers_[GameState::MAIN_MENU] = [this](const std::string& input) {
        return handleMainMenuInput(input);
    };
    
    // Character selection handler
    inputHandlers_[GameState::CHARACTER_SELECT] = [this](const std::string& input) {
        LOG_DEBUG("game", "Character selection handler received input: '" + input + "'");
        try {
            int choice = std::stoi(input);
            LOG_DEBUG("game", "Parsed choice: " + std::to_string(choice));
            
            switch (choice) {
                case 1: // Ironclad
                    LOG_DEBUG("game", "Creating IRONCLAD player");
                    if (createPlayer("ironclad", "")) {
                        LOG_DEBUG("game", "Player created, generating map");
                        if (generateMap(1)) {
                            LOG_DEBUG("game", "Map generated, setting state to MAP");
                            setState(GameState::MAP);
                        } else {
                            LOG_ERROR("game", "Failed to generate map");
                            setState(GameState::MAIN_MENU);
                        }
                    } else {
                        LOG_ERROR("game", "Failed to create IRONCLAD player");
                    }
                    break;
                case 2: // Silent
                    LOG_DEBUG("game", "Creating SILENT player");
                    if (createPlayer("silent", "")) {
                        LOG_DEBUG("game", "Player created, generating map");
                        if (generateMap(1)) {
                            LOG_DEBUG("game", "Map generated, setting state to MAP");
                            setState(GameState::MAP);
                        } else {
                            LOG_ERROR("game", "Failed to generate map");
                            setState(GameState::MAIN_MENU);
                        }
                    } else {
                        LOG_ERROR("game", "Failed to create SILENT player");
                    }
                    break;
                case 3: // Defect
                    LOG_DEBUG("game", "Creating DEFECT player");
                    if (createPlayer("defect", "")) {
                        LOG_DEBUG("game", "Player created, generating map");
                        if (generateMap(1)) {
                            LOG_DEBUG("game", "Map generated, setting state to MAP");
                            setState(GameState::MAP);
                        } else {
                            LOG_ERROR("game", "Failed to generate map");
                            setState(GameState::MAIN_MENU);
                        }
                    } else {
                        LOG_ERROR("game", "Failed to create DEFECT player");
                    }
                    break;
                case 4: // Watcher
                    LOG_DEBUG("game", "Creating WATCHER player");
                    if (createPlayer("watcher", "")) {
                        LOG_DEBUG("game", "Player created, generating map");
                        if (generateMap(1)) {
                            LOG_DEBUG("game", "Map generated, setting state to MAP");
                            setState(GameState::MAP);
                        } else {
                            LOG_ERROR("game", "Failed to generate map");
                            setState(GameState::MAIN_MENU);
                        }
                    } else {
                        LOG_ERROR("game", "Failed to create WATCHER player");
                    }
                    break;
                default:
                    LOG_DEBUG("game", "Invalid character choice: " + std::to_string(choice));
                    ui_->showMessage("Invalid choice. Please select a valid character.", true);
                    ui_->showCharacterSelection({"IRONCLAD", "SILENT", "DEFECT", "WATCHER"});
                    break;
            }
        } catch (const std::exception& e) {
            LOG_ERROR("game", "Exception in character selection handler: " + std::string(e.what()));
            ui_->showMessage("Invalid input. Please enter a number.", true);
            ui_->showCharacterSelection({"IRONCLAD", "SILENT", "DEFECT", "WATCHER"});
        }
        return true;
    };
    
    // Map input handler
    inputHandlers_[GameState::MAP] = [this](const std::string& input) {
        return handleMapInput(input);
    };
    
    // Combat input handler
    inputHandlers_[GameState::COMBAT] = [this](const std::string& input) {
        return handleCombatInput(input);
    };
    
    // Event input handler
    inputHandlers_[GameState::EVENT] = [this](const std::string& input) {
        return handleEventInput(input);
    };
    
    // Game over handler - return to main menu
    inputHandlers_[GameState::GAME_OVER] = [this](const std::string& input) {
        LOG_INFO("game", "Game over handler activated with input: '" + input + "'");
        
        // Calculate final score and log it
        int score = calculateScore();
        LOG_INFO("game", "Final score: " + std::to_string(score));
        
        // Clean up any remaining state
        LOG_INFO("game", "Cleaning up game state after game over");
        currentCombat_.reset();
        transitioningFromCombat_ = false;
        currentEvent_.reset();
        
        // Reset the game state for a new run
        LOG_INFO("game", "Resetting map and player for new game");
        map_.reset();
        player_.reset();
        
        // Return to main menu
        LOG_INFO("game", "Transitioning to main menu after game over");
        setState(GameState::MAIN_MENU);
        
        LOG_INFO("game", "Returned to main menu after game over");
        return true;
    };
    
    // Rewards handler - return to map
    inputHandlers_[GameState::REWARD] = [this](const std::string&) {
        LOG_INFO("game", "Leaving rewards screen, returning to map");
        
        // Clear combat object since we're done with it
        currentCombat_.reset();
        
        // Reset transition flag
        transitioningFromCombat_ = false;
        
        // Return to map immediately
        setState(GameState::MAP);
        return true;
    };
}

bool Game::handleMainMenuInput(const std::string& input) {
    LOG_DEBUG("game", "handleMainMenuInput received input: '" + input + "'");
    try {
        int choice = std::stoi(input);
        switch (choice) {
            case 1: // New Game
                LOG_DEBUG("game", "Setting state to CHARACTER_SELECT");
                setState(GameState::CHARACTER_SELECT);
                LOG_DEBUG("game", "State set to CHARACTER_SELECT");
                break;
            case 2: // Exit
                LOG_DEBUG("game", "Shutting down");
                shutdown();
                break;
            default:
                ui_->showMessage("Invalid choice. Please enter 1 or 2.", true);
                ui_->showMainMenu();
                break;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("game", "Exception in handleMainMenuInput: " + std::string(e.what()));
        ui_->showMessage("Invalid input. Please enter a number.", true);
        ui_->showMainMenu();
    }
    
    return true;
}

bool Game::handleMapInput(const std::string& input) {
    if (!map_) {
        return false;
    }
    
    // If input is empty, just redisplay the map
    if (input.empty()) {
        LOG_INFO("game", "Empty input in map view, redisplaying map");
        // Get current room ID
        int currentRoomId = -1;
        if (map_->getCurrentRoom()) {
            currentRoomId = map_->getCurrentRoom()->id;
        }
        ui_->showMap(currentRoomId, map_->getAvailableRooms(), map_->getAllRooms());
        return true;
    }
    
    if (input == "back") {
        ui_->showMainMenu();
        return true;
    }
    
    try {
        // First check if input contains only digits to avoid exceptions
        bool isValidNumber = !input.empty() && std::all_of(input.begin(), input.end(), 
            [](char c) { return std::isdigit(c); });
            
        if (!isValidNumber) {
            LOG_INFO("game", "Invalid map input: '" + input + "' (not a number)");
            // Get current room ID
            int currentRoomId = -1;
            if (map_->getCurrentRoom()) {
                currentRoomId = map_->getCurrentRoom()->id;
            }
            ui_->showMessage("Please enter a valid room number.", true);
            ui_->showMap(currentRoomId, map_->getAvailableRooms(), map_->getAllRooms());
            return true;
        }
        
        // Convert displayed index to room ID
        int selectedIndex = std::stoi(input) - 1; // Convert to zero-based index
        
        // Get available rooms
        std::vector<int> availableRooms = map_->getAvailableRooms();
        
        // Debug logging - print all rooms
        LOG_INFO("game", "Current room ID: " + std::to_string(map_->getCurrentRoom() ? map_->getCurrentRoom()->id : -1));
        LOG_INFO("game", "Available rooms count: " + std::to_string(availableRooms.size()));
        LOG_INFO("game", "User selected index: " + std::to_string(selectedIndex) + " (from input: " + input + ")");
        const auto& allRooms = map_->getAllRooms();
        LOG_INFO("game", "Total rooms in map: " + std::to_string(allRooms.size()));
        
        for (const auto& [id, room] : allRooms) {
            LOG_INFO("game", "Room #" + std::to_string(id) + 
                    ": Type=" + std::to_string(static_cast<int>(room.type)) + 
                    ", Floor=" + std::to_string(room.y) + 
                    ", Visited=" + (room.visited ? "true" : "false") +
                    ", Connected to " + std::to_string(room.nextRooms.size()) + " rooms");
            
            if (id == map_->getCurrentRoom()->id) {
                LOG_INFO("game", "This is the current room");
                LOG_INFO("game", "Next rooms for current room:");
                for (int nextId : room.nextRooms) {
                    auto nextIt = allRooms.find(nextId);
                    if (nextIt != allRooms.end()) {
                        LOG_INFO("game", "  Room #" + std::to_string(nextId) + 
                                ": Type=" + std::to_string(static_cast<int>(nextIt->second.type)) + 
                                ", Visited=" + (nextIt->second.visited ? "true" : "false"));
                    }
                }
            }
        }
        
        // Check if index is valid
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(availableRooms.size())) {
            int roomId = availableRooms[selectedIndex];
            
            if (map_->canMoveTo(roomId)) {
                // Move to room
                if (map_->moveToRoom(roomId)) {
                    // Mark the room as visited
                    map_->markCurrentRoomVisited();
                    
                    // Get current room
                    const Room* room = map_->getCurrentRoom();
                    if (room) {
                        // Handle room based on type
                        switch (room->type) {
                            case RoomType::MONSTER: {
                                // Get a list of all available enemies appropriate for the current floor range
                                std::vector<std::string> availableEnemies;
                                int floorRange = map_->getEnemyFloorRange(); // Get floor range for enemy selection
                                
                                LOG_INFO("game", "Selecting enemy for monster room at floor range: " + std::to_string(floorRange));
                                
                                try {
                                    for (const auto& entry : fs::directory_iterator("data/enemies")) {
                                        if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                            // Load the enemy data to check if it's appropriate for this floor
                                            std::string id = entry.path().stem().string();
                                            json enemyData;
                                            std::ifstream file(entry.path());
                                            if (file.is_open()) {
                                                file >> enemyData;
                                                
                                                // Check if enemy is appropriate for this floor range
                                                bool isAppropriate = true;
                                                
                                                // Skip elites and bosses for monster rooms
                                                if ((enemyData.contains("is_elite") && enemyData["is_elite"].get<bool>()) || 
                                                    (enemyData.contains("is_boss") && enemyData["is_boss"].get<bool>())) {
                                                    isAppropriate = false;
                                                }
                                                
                                                // Check min/max floor range if specified
                                                if (enemyData.contains("min_floor") && 
                                                    floorRange < enemyData["min_floor"].get<int>()) {
                                                    isAppropriate = false;
                                                }
                                                
                                                if (enemyData.contains("max_floor") && 
                                                    floorRange > enemyData["max_floor"].get<int>()) {
                                                    isAppropriate = false;
                                                }
                                                
                                                if (isAppropriate) {
                                                    availableEnemies.push_back(id);
                                                    LOG_DEBUG("game", "Added enemy " + id + " to available list for floor range " + 
                                                              std::to_string(floorRange));
                                                }
                                            }
                                        }
                                    }
                                } catch (const std::exception& e) {
                                    LOG_ERROR("game", "Error loading enemies for encounter: " + std::string(e.what()));
                                }
                                
                                if (availableEnemies.empty()) {
                                    LOG_WARNING("game", "No appropriate enemies found for floor range " + std::to_string(floorRange) + 
                                                ", falling back to all non-elite enemies");
                                    
                                    // Fallback: Get all non-elite, non-boss enemies
                                    try {
                                        for (const auto& entry : fs::directory_iterator("data/enemies")) {
                                            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                                try {
                                                    std::string id = entry.path().stem().string();
                                                    json enemyData;
                                                    std::ifstream file(entry.path());
                                                    if (file.is_open()) {
                                                        file >> enemyData;
                                                        if ((!enemyData.contains("is_elite") || !enemyData["is_elite"].get<bool>()) && 
                                                            (!enemyData.contains("is_boss") || !enemyData["is_boss"].get<bool>())) {
                                                            availableEnemies.push_back(id);
                                                        }
                                                    }
                                                } catch (const std::exception& e) {
                                                    LOG_ERROR("game", "Error loading enemy file: " + std::string(e.what()));
                                                }
                                            }
                                        }
                                    } catch (const std::exception& e) {
                                        LOG_ERROR("game", "Error accessing enemy directory: " + std::string(e.what()));
                                    }
                                }
                                
                                if (availableEnemies.empty()) {
                                    ui_->showMessage("Error: No enemies found for this floor.", true);
                                    ui_->showMap(roomId, map_->getAvailableRooms(), map_->getAllRooms());
                                    return true;
                                }
                                
                                // Choose a random appropriate enemy
                                unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                                std::mt19937 gen(seed);
                                std::uniform_int_distribution<> dist(0, availableEnemies.size() - 1);
                                int enemyIndex = dist(gen);
                                
                                LOG_INFO("game", "Selected enemy: " + availableEnemies[enemyIndex] + " for floor range " + 
                                          std::to_string(floorRange));
                                
                                startCombat({availableEnemies[enemyIndex]});
                                break;
                            }
                            case RoomType::ELITE: {
                                // Get a list of all available elite enemies appropriate for the current floor range
                                std::vector<std::string> availableElites;
                                int floorRange = map_->getEnemyFloorRange();
                                
                                LOG_INFO("game", "Selecting elite enemy for elite room at floor range: " + std::to_string(floorRange));
                                
                                try {
                                    for (const auto& entry : fs::directory_iterator("data/enemies")) {
                                        if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                            try {
                                                std::string id = entry.path().stem().string();
                                                json enemyData;
                                                std::ifstream file(entry.path());
                                                if (file.is_open()) {
                                                    file >> enemyData;
                                                    
                                                    // Check if this is an elite appropriate for this floor range
                                                    bool isAppropriate = true;
                                                    
                                                    // Must be an elite
                                                    if (!enemyData.contains("is_elite") || !enemyData["is_elite"].get<bool>()) {
                                                        isAppropriate = false;
                                                    }
                                                    
                                                    // Check min/max floor range if specified
                                                    if (enemyData.contains("min_floor") && 
                                                        floorRange < enemyData["min_floor"].get<int>()) {
                                                        isAppropriate = false;
                                                    }
                                                    
                                                    if (enemyData.contains("max_floor") && 
                                                        floorRange > enemyData["max_floor"].get<int>()) {
                                                        isAppropriate = false;
                                                    }
                                                    
                                                    if (isAppropriate) {
                                                        availableElites.push_back(id);
                                                        LOG_DEBUG("game", "Added elite enemy " + id + " to available list for floor range " + 
                                                                std::to_string(floorRange));
                                                    }
                                                }
                                            } catch (const std::exception& e) {
                                                LOG_ERROR("game", "Error loading elite enemy file: " + std::string(e.what()));
                                            }
                                        }
                                    }
                                } catch (const std::exception& e) {
                                    LOG_ERROR("game", "Error accessing elite enemy directory: " + std::string(e.what()));
                                }
                                
                                // If no appropriate elites found, try all elites
                                if (availableElites.empty()) {
                                    LOG_WARNING("game", "No appropriate elite enemies found for floor range " + std::to_string(floorRange) + 
                                                ", falling back to all elite enemies");
                                    
                                    try {
                                        for (const auto& entry : fs::directory_iterator("data/enemies")) {
                                            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                                try {
                                                    std::string id = entry.path().stem().string();
                                                    json enemyData;
                                                    std::ifstream file(entry.path());
                                                    if (file.is_open()) {
                                                        file >> enemyData;
                                                        if (enemyData.contains("is_elite") && enemyData["is_elite"].get<bool>()) {
                                                            availableElites.push_back(id);
                                                        }
                                                    }
                                                } catch (const std::exception& e) {
                                                    LOG_ERROR("game", "Error loading elite enemy file: " + std::string(e.what()));
                                                }
                                            }
                                        }
                                    } catch (const std::exception& e) {
                                        LOG_ERROR("game", "Error in fallback elite loading: " + std::string(e.what()));
                                    }
                                }
                                
                                // If still no elites, fall back to multiple basic enemies
                                if (availableElites.empty()) {
                                    LOG_WARNING("game", "No elite enemies found, falling back to multiple basic enemies");
                                    
                                    // Fall back to multiple basic enemies
                                    std::vector<std::string> basicEnemies;
                                    try {
                                        for (const auto& entry : fs::directory_iterator("data/enemies")) {
                                            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                                try {
                                                    std::string id = entry.path().stem().string();
                                                    json enemyData;
                                                    std::ifstream file(entry.path());
                                                    if (file.is_open()) {
                                                        file >> enemyData;
                                                        if ((!enemyData.contains("is_elite") || 
                                                             !enemyData["is_elite"].get<bool>()) && 
                                                            (!enemyData.contains("is_boss") || 
                                                             !enemyData["is_boss"].get<bool>())) {
                                                            basicEnemies.push_back(id);
                                                        }
                                                    }
                                                } catch (const std::exception& e) {
                                                    LOG_ERROR("game", "Error loading basic enemy file: " + std::string(e.what()));
                                                }
                                            }
                                        }
                                    } catch (const std::exception& e) {
                                                LOG_ERROR("game", "Error in fallback basic enemy loading: " + std::string(e.what()));
                                            }
                                    
                                    if (basicEnemies.empty()) {
                                        ui_->showMessage("Error: No enemies found for elite encounter.", true);
                                        ui_->showMap(roomId, map_->getAvailableRooms(), map_->getAllRooms());
                                        return true;
                                    }
                                    
                                    try {
                                        // Choose two different basic enemies if possible
                                        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                                        std::mt19937 gen(seed);
                                        std::vector<std::string> encounter;
                                        
                                        std::uniform_int_distribution<> dist(0, basicEnemies.size() - 1);
                                        int firstEnemyIndex = dist(gen);
                                        encounter.push_back(basicEnemies[firstEnemyIndex]);
                                        
                                        if (basicEnemies.size() > 1) {
                                            // Choose a different enemy for the second one
                                            std::vector<std::string> remainingEnemies = basicEnemies;
                                            remainingEnemies.erase(remainingEnemies.begin() + firstEnemyIndex);
                                            std::uniform_int_distribution<> dist2(0, remainingEnemies.size() - 1);
                                            encounter.push_back(remainingEnemies[dist2(gen)]);
                                        } else {
                                            // Only one enemy available, use it twice
                                            encounter.push_back(basicEnemies[0]);
                                        }
                                        
                                        startCombat(encounter);
                                    } catch (const std::exception& e) {
                                        LOG_ERROR("game", "Error creating elite encounter: " + std::string(e.what()));
                                        ui_->showMessage("Error: Failed to create encounter.", true);
                                        ui_->showMap(roomId, map_->getAvailableRooms(), map_->getAllRooms());
                                        return true;
                                    }
                                } else {
                                    // Choose a random elite enemy
                                    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                                    std::mt19937 gen(seed);
                                    std::uniform_int_distribution<> dist(0, availableElites.size() - 1);
                                    int enemyIndex = dist(gen);
                                    
                                    LOG_INFO("game", "Selected elite enemy: " + availableElites[enemyIndex] + " for floor range " + 
                                             std::to_string(floorRange));
                                    
                                    startCombat({availableElites[enemyIndex]});
                                }
                                break;
                            }
                            case RoomType::BOSS: {
                                // Get a list of all available boss enemies
                                std::vector<std::string> bossEnemies;
                                try {
                                    for (const auto& entry : fs::directory_iterator("data/enemies")) {
                                        if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                            try {
                                                // Load the enemy data to check if it's a boss enemy
                                                std::string id = entry.path().stem().string();
                                                json enemyData;
                                                std::ifstream file(entry.path());
                                                if (file.is_open()) {
                                                    file >> enemyData;
                                                    // Include boss enemies
                                                    if (enemyData.contains("is_boss") && 
                                                        enemyData["is_boss"].get<bool>()) {
                                                        bossEnemies.push_back(id);
                                                    }
                                                }
                                            } catch (const std::exception& e) {
                                                LOG_ERROR("game", "Error loading boss enemy file: " + std::string(e.what()));
                                            }
                                        }
                                    }
                                } catch (const std::exception& e) {
                                    LOG_ERROR("game", "Error loading boss enemies for encounter: " + std::string(e.what()));
                                }
                                
                                if (bossEnemies.empty()) {
                                    ui_->showMessage("Error: No boss enemies found.", true);
                                    ui_->showMap(roomId, map_->getAvailableRooms(), map_->getAllRooms());
                                    return true;
                                }
                                
                                // Choose a random boss enemy
                                unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                                std::mt19937 gen(seed);
                                std::uniform_int_distribution<> dist(0, bossEnemies.size() - 1);
                                int enemyIndex = dist(gen);
                                startCombat({bossEnemies[enemyIndex]});
                                break;
                            }
                            case RoomType::EVENT: {
                                // Get a random event
                                if (eventTemplates_.empty()) {
                                    ui_->showMessage("Error: No events found.", true);
                                    ui_->showMap(roomId, map_->getAvailableRooms(), map_->getAllRooms());
                                    return true;
                                }
                                
                                // Select a random event
                                std::vector<std::string> eventIds;
                                for (const auto& [id, event] : eventTemplates_) {
                                    eventIds.push_back(id);
                                }
                                
                                unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                                std::mt19937 gen(seed);
                                std::uniform_int_distribution<> dist(0, eventIds.size() - 1);
                                int eventIndex = dist(gen);
                                
                                startEvent(eventIds[eventIndex]);
                                break;
                            }
                            case RoomType::REST: {
                                // Start the rest site event
                                startEvent("rest_site");
                                break;
                            }
                            default:
                                // For now, just show the map again
                                int currentRoomId = room->id;
                                ui_->showMap(currentRoomId, map_->getAvailableRooms(), map_->getAllRooms());
                                break;
                        }
                        
                        return true;
                    }
                }
            }
        }
        
        // Get current room ID
        int currentRoomId = -1;
        if (map_->getCurrentRoom()) {
            currentRoomId = map_->getCurrentRoom()->id;
        }
        
        ui_->showMessage("Cannot move to that room.", true);
        ui_->showMap(currentRoomId, map_->getAvailableRooms(), map_->getAllRooms());
    } catch (const std::exception&) {
        // Get current room ID
        int currentRoomId = -1;
        if (map_->getCurrentRoom()) {
            currentRoomId = map_->getCurrentRoom()->id;
        }
        
        ui_->showMessage("Invalid room number.", true);
        ui_->showMap(currentRoomId, map_->getAvailableRooms(), map_->getAllRooms());
    }
    
    return true;
}

bool Game::handleCombatInput(const std::string& input) {
    if (!currentCombat_ || !player_) {
        LOG_ERROR("game", "handleCombatInput called without combat or player");
        return false;
    }
    
    // If we're already transitioning to rewards, skip processing
    if (transitioningFromCombat_) {
        LOG_DEBUG("game", "Already transitioning from combat, skipping input processing");
        return true;
    }
    
    // Check if combat is over - if so, immediately show rewards and skip further input processing
    if (currentCombat_->isCombatOver()) {
        LOG_INFO("game", "Combat is over, transitioning. Player defeated: " + 
            std::string(currentCombat_->isPlayerDefeated() ? "true" : "false"));
            
        // Call endCombat with the appropriate victory state
        endCombat(!currentCombat_->isPlayerDefeated());
        return true;
    }
    
    // If we're waiting for enemy selection, handle that first
    if (awaitingEnemySelection_) {
        if (input == "cancel" || input == "c") {
            // User canceled the selection, go back to normal combat view
            awaitingEnemySelection_ = false;
            selectedCardIndex_ = -1;
            ui_->showCombat(currentCombat_.get());
            return true;
        }
        
        try {
            // Parse enemy index
            int targetIndex = std::stoi(input) - 1;
            
            // Check if the card can be played on this target
            const auto& hand = player_->getHand();
            if (selectedCardIndex_ >= 0 && selectedCardIndex_ < static_cast<int>(hand.size())) {
                const std::string& cardName = hand[selectedCardIndex_]->getName();
                
                if (currentCombat_->playCard(selectedCardIndex_, targetIndex)) {
                    // Reset selection state
                    awaitingEnemySelection_ = false;
                    selectedCardIndex_ = -1;
                    ui_->showCombat(currentCombat_.get());
                } else {
                    ui_->showMessage("Cannot target that enemy with " + cardName + ". Try a different target or card.", true);
                    // Show the enemy selection menu again
                    ui_->showEnemySelectionMenu(currentCombat_.get(), cardName);
                }
            } else {
                // Shouldn't happen, but handle it anyway
                awaitingEnemySelection_ = false;
                selectedCardIndex_ = -1;
                ui_->showCombat(currentCombat_.get());
            }
        } catch (const std::exception&) {
            ui_->showMessage("Invalid target. Please enter a valid enemy number or 'cancel'.", true);
            // Show the enemy selection menu again with the same card
            const auto& hand = player_->getHand();
            if (selectedCardIndex_ >= 0 && selectedCardIndex_ < static_cast<int>(hand.size())) {
                ui_->showEnemySelectionMenu(currentCombat_.get(), hand[selectedCardIndex_]->getName());
            } else {
                // Shouldn't happen, but handle it anyway
                awaitingEnemySelection_ = false;
                selectedCardIndex_ = -1;
                ui_->showCombat(currentCombat_.get());
            }
        }
        
        return true;
    }
    
    // Normal combat input processing
    if (input == "end" || input == "e") {
        // End turn
        currentCombat_->endPlayerTurn();
        ui_->showCombat(currentCombat_.get());
    } else if (input == "help" || input == "h") {
        // Show help
        ui_->showMessage(
            "Combat Commands:\n\n"
            "  <number>        - Play the card with that number\n"
            "                     If the card needs a target, you'll be prompted to select an enemy\n"
            "                     (Cards will auto-target if there's only one enemy)\n"
            "  <number> <num>  - Play the card directly on a specific enemy\n"
            "                     (e.g., '1 2' to play the first card on the second enemy)\n"
            "  end or e        - End your turn\n"
            "  help or h       - Show this help message\n"
            "  cancel or c     - Cancel enemy selection\n\n"
            "Tips:\n"
            "  - Block protects against damage but is reset at the end of your turn\n"
            "  - Energy is replenished each turn\n"
            "  - Cards with 'Exhaust' are removed from your deck for the rest of combat after playing them", 
            true
        );
        ui_->showCombat(currentCombat_.get());
    } else {
        // Check if the input is a number (card to play)
        try {
            size_t spacePos = input.find(' ');
            std::string cardStr = input.substr(0, spacePos);
            
            // Convert to integer
            int cardIndex = std::stoi(cardStr) - 1;
            
            const auto& hand = player_->getHand();
            if (cardIndex < 0 || cardIndex >= static_cast<int>(hand.size())) {
                ui_->showMessage("Invalid card number. Please enter a number between 1 and " + 
                                std::to_string(hand.size()) + ".", true);
                ui_->showCombat(currentCombat_.get());
                return true;
            }
            
            // Check if we have a target index in the same command
            if (spacePos != std::string::npos) {
                // Direct targeting: <card> <enemy>
                int targetIndex = std::stoi(input.substr(spacePos + 1)) - 1;
                
                // Play the card with the specified target
                if (currentCombat_->playCard(cardIndex, targetIndex)) {
                    ui_->showCombat(currentCombat_.get());
                } else {
                    ui_->showMessage("Cannot play that card on that target. Check energy cost or valid targets.", true);
                    ui_->showCombat(currentCombat_.get());
                }
            } else {
                // Two-step targeting: first select card, then enemy
                Card* card = hand[cardIndex].get();
                
                // If card needs a target, go to enemy selection mode
                if (card && card->needsTarget()) {
                    // Check if there's only one enemy - auto-target in that case
                    if (currentCombat_->getEnemyCount() == 1 && currentCombat_->getEnemy(0) && currentCombat_->getEnemy(0)->isAlive()) {
                        // Only one enemy, auto-target it
                        if (currentCombat_->playCard(cardIndex, 0)) {
                            ui_->showCombat(currentCombat_.get());
                        } else {
                            ui_->showMessage("Cannot play " + card->getName() + " on the enemy. Check energy cost.", true);
                            ui_->showCombat(currentCombat_.get());
                        }
                    } else {
                        // Multiple enemies, show selection menu
                        awaitingEnemySelection_ = true;
                        selectedCardIndex_ = cardIndex;
                        ui_->showEnemySelectionMenu(currentCombat_.get(), card->getName());
                    }
                } else {
                    // Card doesn't need a target, play it directly
                    if (currentCombat_->playCard(cardIndex)) {
                        ui_->showCombat(currentCombat_.get());
                    } else {
                        ui_->showMessage("Cannot play " + card->getName() + ". Check energy cost.", true);
                        ui_->showCombat(currentCombat_.get());
                    }
                }
            }
        } catch (const std::exception&) {
            ui_->showMessage("Invalid command. Type 'help' to see available commands.", true);
            ui_->showCombat(currentCombat_.get());
        }
    }
    
    return true;
}

std::shared_ptr<Event> Game::loadEvent(const std::string& id) {
    // Check if already loaded
    auto it = eventTemplates_.find(id);
    if (it != eventTemplates_.end()) {
        // Return a clone of the template
        return std::shared_ptr<Event>(static_cast<Event*>(it->second->clone().release()));
    }
    
    // Load from file
    std::string eventPath = "data/events/" + id + ".json";
    try {
        std::ifstream file(eventPath);
        if (!file.is_open()) {
            LOG_ERROR("game", "Failed to open event file: " + eventPath);
            return nullptr;
        }
        
        nlohmann::json eventData;
        file >> eventData;
        
        auto event = std::make_shared<Event>();
        if (event->loadFromJson(eventData)) {
            // Add to templates
            eventTemplates_[id] = std::shared_ptr<Event>(static_cast<Event*>(event->clone().release()));
            return event;
        } else {
            LOG_ERROR("game", "Failed to load event from JSON: " + id);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("game", "Error loading event: " + std::string(e.what()));
    }
    
    return nullptr;
}

bool Game::loadAllEvents() {
    LOG_INFO("game", "Loading all events...");
    try {
        // Search data/events directory for .json files
        namespace fs = std::filesystem;
        if (!fs::exists("data/events")) {
            LOG_ERROR("game", "Events directory not found");
            return false;
        }
        
        for (const auto& entry : fs::directory_iterator("data/events")) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string id = entry.path().stem().string();
                auto event = loadEvent(id);
                if (!event) {
                    LOG_ERROR("game", "Failed to load event: " + id);
                }
            }
        }
        
        LOG_INFO("game", "Loaded " + std::to_string(eventTemplates_.size()) + " events");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("game", "Error loading events: " + std::string(e.what()));
        return false;
    }
}

bool Game::startEvent(const std::string& eventId) {
    LOG_INFO("game", "Starting event: " + eventId);
    
    // Load event
    currentEvent_ = loadEvent(eventId);
    if (!currentEvent_) {
        LOG_ERROR("game", "Failed to load event: " + eventId);
        return false;
    }
    
    // Set game state to EVENT
    setState(GameState::EVENT);
    return true;
}

void Game::endEvent() {
    LOG_INFO("game", "Ending event");
    
    // Clear current event
    currentEvent_ = nullptr;
    
    // Mark current room as visited
    if (map_) {
        map_->markCurrentRoomVisited();
    }
    
    // Return to map
    setState(GameState::MAP);
}

bool Game::handleEventInput(const std::string& input) {
    if (!currentEvent_ || !player_) {
        return false;
    }
    
    try {
        // Check if input is "back" to return to map
        if (input == "back") {
            endEvent();
            return true;
        }
        
        // Try to parse input as a choice index
        int choiceIndex = std::stoi(input) - 1; // Convert to zero-based index
        
        // Get available choices
        std::vector<EventChoice> availableChoices = currentEvent_->getAvailableChoices(player_.get());
        
        // Check if choice is valid
        if (choiceIndex >= 0 && choiceIndex < static_cast<int>(availableChoices.size())) {
            // Process choice
            std::string resultText = currentEvent_->processChoice(choiceIndex, this);
            
            // Show result
            ui_->showEventResult(resultText);
            
            // End event
            endEvent();
        } else {
            ui_->showMessage("Invalid choice. Please select a valid option.", true);
            ui_->showEvent(currentEvent_.get(), player_.get());
        }
    } catch (const std::exception&) {
        ui_->showMessage("Invalid input. Please enter a number or 'back'.", true);
        ui_->showEvent(currentEvent_.get(), player_.get());
    }
    
    return true;
}

bool Game::loadGameData() {
    // Create data directories if they don't exist
    std::vector<std::string> dataDirs = {
        "data",
        "data/cards",
        "data/enemies",
        "data/relics",
        "data/characters",
        "data/events"
    };
    
    for (const auto& dir : dataDirs) {
        if (!fs::exists(dir)) {
            try {
                fs::create_directory(dir);
            } catch (const std::exception& e) {
                std::cerr << "Failed to create directory: " << dir << ": " << e.what() << std::endl;
                return false;
            }
        }
    }
    
    // Load all game data
    if (!loadAllCards()) {
        std::cerr << "Failed to load cards." << std::endl;
        return false;
    }
    
    if (!loadAllEnemies()) {
        std::cerr << "Failed to load enemies." << std::endl;
        return false;
    }
    
    if (!loadAllRelics()) {
        std::cerr << "Failed to load relics." << std::endl;
        return false;
    }
    
    if (!loadAllEvents()) {
        std::cerr << "Failed to load events." << std::endl;
        return false;
    }
    
    return true;
}

// Add methods to support event effects
bool Game::addCardToDeck(const std::string& cardId) {
    auto card = loadCard(cardId);
    if (card && player_) {
        player_->addCard(card);
        return true;
    }
    return false;
}

bool Game::addRelic(const std::string& relicId) {
    auto relic = loadRelic(relicId);
    if (relic && player_) {
        player_->addRelic(relic);
        return true;
    }
    return false;
}

// New method to show card selection UI and upgrade a card
std::string Game::upgradeCard() {
    if (!player_) {
        LOG_ERROR("game", "Cannot upgrade card: player is null");
        return "Unknown card";
    }
    
    // Get all upgradable cards from player's deck
    std::vector<std::shared_ptr<Card>> upgradeableCards;
    std::vector<int> cardIndices;
    
    // Collect cards from all piles
    std::vector<std::shared_ptr<Card>> allCards;
    // Add draw pile cards
    allCards.insert(allCards.end(), player_->getDrawPile().begin(), player_->getDrawPile().end());
    // Add discard pile cards
    allCards.insert(allCards.end(), player_->getDiscardPile().begin(), player_->getDiscardPile().end());
    // Add hand cards
    allCards.insert(allCards.end(), player_->getHand().begin(), player_->getHand().end());
    
    // Filter upgradable cards
    for (size_t i = 0; i < allCards.size(); i++) {
        if (allCards[i] && !allCards[i]->isUpgraded() && allCards[i]->isUpgradable()) {
            upgradeableCards.push_back(allCards[i]);
            cardIndices.push_back(i);
        }
    }
    
    if (upgradeableCards.empty()) {
        LOG_WARNING("game", "No upgradable cards found");
        return "None (no upgradable cards available)";
    }
    
    // Convert to raw pointers for the UI
    std::vector<Card*> displayCards;
    for (auto& card : upgradeableCards) {
        displayCards.push_back(card.get());
    }
    
    // Show UI for card selection
    ui_->showCards(displayCards, "Select a card to upgrade", true);
    
    // Get user input
    std::string input = ui_->getInput("Enter card number to upgrade (1-" + 
                                     std::to_string(upgradeableCards.size()) + "): ");
    
    try {
        int cardIndex = std::stoi(input) - 1;
        if (cardIndex >= 0 && cardIndex < static_cast<int>(upgradeableCards.size())) {
            // Upgrade the selected card
            std::shared_ptr<Card> selectedCard = upgradeableCards[cardIndex];
            std::string cardName = selectedCard->getName();
            
            if (selectedCard->upgrade()) {
                LOG_INFO("game", "Upgraded card: " + cardName);
                return cardName;
            } else {
                LOG_ERROR("game", "Failed to upgrade card: " + cardName);
                return "Failed to upgrade " + cardName;
            }
        } else {
            LOG_ERROR("game", "Invalid card index: " + std::to_string(cardIndex));
            return "Unknown card";
        }
    } catch (const std::exception& e) {
        LOG_ERROR("game", "Error upgrading card: " + std::string(e.what()));
        return "Unknown card";
    }
}

/**
 * @brief Calculate the player's score based on their progress
 * @return The calculated score
 */
int Game::calculateScore() const {
    int score = 0;
    
    // If no player, return 0
    if (!player_) {
        return score;
    }
    
    // Base score from player's gold
    score += player_->getGold();
    
    // Add points for player's health
    score += player_->getHealth() * 2;
    
    // Add points for map progress
    if (map_) {
        // Add points for current floor
        const Room* currentRoom = map_->getCurrentRoom();
        if (currentRoom) {
            score += currentRoom->y * 10;
        }
        
        // Add points for act
        score += map_->getAct() * 100;
        
        // Add points for boss defeated
        if (map_->isBossDefeated()) {
            score += 500;
        }
    }
    
    // Add points for relics
    const auto& relics = player_->getRelics();
    score += relics.size() * 50;
    
    // Add points for deck size
    const auto& drawPile = player_->getDrawPile();
    const auto& discardPile = player_->getDiscardPile();
    const auto& hand = player_->getHand();
    int deckSize = drawPile.size() + discardPile.size() + hand.size();
    score += deckSize * 5;
    
    LOG_INFO("game", "Calculated final score: " + std::to_string(score));
    return score;
}

} // namespace deckstiny 