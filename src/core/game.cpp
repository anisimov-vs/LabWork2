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
#include "util/path_util.h"

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

std::string GameStateToString(GameState state) {
    switch (state) {
        case GameState::MAIN_MENU: return "MAIN_MENU";
        case GameState::CHARACTER_SELECT: return "CHARACTER_SELECT";
        case GameState::MAP: return "MAP";
        case GameState::COMBAT: return "COMBAT";
        case GameState::EVENT: return "EVENT";
        case GameState::SHOP: return "SHOP";
        case GameState::REWARD: return "REWARD";
        case GameState::CARD_SELECT: return "CARD_SELECT";
        case GameState::REST: return "REST";
        case GameState::GAME_OVER: return "GAME_OVER";
        default: return "UNKNOWN_GAME_STATE (" + std::to_string(static_cast<int>(state)) + ")";
    }
}

Game::Game() 
    : state_(GameState::MAIN_MENU) {
}

Game::~Game() {
    util::Logger::getInstance().log(util::LogLevel::Info, "game", "Game shutting down");
}

void Game::initializeLogging() {
    util::Logger::init();
    util::Logger::getInstance().setFileEnabled(true); 
    util::Logger::getInstance().setLogDirectory("logs/deckstiny"); 
    util::Logger::getInstance().setConsoleEnabled(true); 
    util::Logger::getInstance().setFileLevel(util::LogLevel::Debug);
    util::Logger::getInstance().setConsoleLevel(util::LogLevel::Warning);
}

bool Game::initialize(std::shared_ptr<UIInterface> uiInterface) {
    initializeLogging();

    if (!uiInterface) {
        LOG_ERROR("game", "UIInterface is null, cannot initialize game.");
        return false;
    }
    ui_ = uiInterface;
    LOG_INFO("game", "Game::initialize called. UIInterface assigned. Relying on external logger configuration.");

    srand(time(0));
    
    if (!ui_->initialize(this)) {
        LOG_ERROR("system", "Failed to initialize UI");
        return false;
    }
    
    LOG_DEBUG("system", "UI initialized successfully");
    
    ui_->setInputCallback([this](const std::string& input) {
        return processInput(input);
    });
    
    LOG_DEBUG("system", "Input callback registered");
    
    initializeInputHandlers();
    LOG_DEBUG("system", "Input handlers initialized");
    
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    rng_.seed(seed);

    if (!loadGameData()) {
        LOG_ERROR("game", "Failed to load essential game data during Game::initialize.");
        return false;
    }
    
    LOG_INFO("system", "Game initialization completed successfully");
    return true;
}

void Game::run() {
    LOG_INFO("game", "Starting game loop");
    running_ = true;
    
    setState(GameState::MAIN_MENU);
    
    LOG_INFO("game", "Game loop started");
    
    while (running_) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    LOG_INFO("game", "Game loop ended");
}

void Game::shutdown() {
    LOG_INFO("game", "Shutting down game");
    running_ = false;
    LOG_INFO("game", "Game shutdown complete");
}

void Game::setState(GameState newState) {
    LOG_DEBUG("game", "Attempting to change state from " + GameStateToString(state_) +
             " to " + GameStateToString(newState));

    GameState previousState = state_;
    state_ = newState;

    LOG_DEBUG("game", "newState value before switch: " + GameStateToString(newState) + ", current state_ after assignment: " + GameStateToString(state_));

    switch (newState) {
        case GameState::MAIN_MENU: {
            LOG_DEBUG("game", "Showing main menu (switched on newState)");
            if (ui_) ui_->showMainMenu();
            break;
        }
        case GameState::CHARACTER_SELECT: {
            LOG_DEBUG("game", "Showing character selection menu (switched on newState)");
            if (ui_) {
                std::vector<std::string> characterNames;
                if (allCharacters_.empty()) {
                    LOG_WARNING("game_setState", "No characters loaded when trying to show character selection. Attempting to load them now.");
                    loadAllCharacters();
                }
                for(const auto& pair : allCharacters_) {
                    characterNames.push_back(pair.second.name);
                }
                if (characterNames.empty()) {
                    LOG_ERROR("game_setState", "Still no characters available after trying to load. Displaying empty selection.");
                    ui_->showMessage("CRITICAL: No characters found. Please check data files.", true);
                }
                ui_->showCharacterSelection(characterNames);
            }
            LOG_DEBUG("game", "Character selection menu shown");
            break;
        }
        case GameState::MAP: {
            LOG_DEBUG("game", "Showing map (switched on newState)");
            if (ui_ && map_ && map_->getCurrentRoom()) {
                ui_->showMap(map_->getCurrentRoom()->id, map_->getAvailableRooms(), map_->getAllRooms());
                LOG_DEBUG("game", "Map shown");
            } else {
                LOG_ERROR("game", "Cannot show map: UI, map, or current room is null");
                 if (previousState != GameState::MAP) setState(previousState);
            }
            break;
        }
        case GameState::COMBAT: {
            LOG_DEBUG("game", "Showing combat (switched on newState)");
            if (ui_ && currentCombat_) {
                ui_->showCombat(currentCombat_.get());
                LOG_DEBUG("game", "Combat shown");
            } else {
                LOG_ERROR("game", "Cannot show combat: UI or combat is null");
                if (previousState != GameState::COMBAT) setState(previousState);
            }
            break;
        }
        case GameState::SHOP: { 
            LOG_DEBUG("game", "Entering SHOP state (switched on newState)");
            this->startShop(); 
            if (ui_ && player_) {
                LOG_DEBUG("game_setState_shop_check", "Before calling ui_->showShop: shopCardsForSale_.size() = " + std::to_string(shopCardsForSale_.size()) + ", shopRelicsForSale_.size() = " + std::to_string(shopRelicsForSale_.size()));
                ui_->showShop(shopCardsForSale_, shopRelicsForSale_, shopRelicPrices_, player_->getGold());
                LOG_DEBUG("game", "Shop shown");
            } else {
                LOG_ERROR("game", "Cannot show shop: UI or player is null");
                if (previousState != GameState::SHOP) setState(previousState);
            }
            break;
        }
        case GameState::EVENT: {
            LOG_DEBUG("game", "Showing event (switched on newState)");
            if (ui_ && currentEvent_ && player_) {
                ui_->showEvent(currentEvent_.get(), player_.get());
            } else if (map_ && map_->getCurrentRoom() && map_->getCurrentRoom()->type == RoomType::EVENT) {
                if (allEvents_.empty()) {
                    LOG_ERROR("game", "No events loaded for event room.");
                    setState(GameState::MAP); 
                    break;
                }
                if (!allEvents_.empty()) {
                   startEvent(allEvents_.begin()->first);
            } else {
                    LOG_ERROR("game", "No events loaded to start (logic error).");
                    setState(GameState::MAP);
                }
            } else {
                LOG_ERROR("game", "In EVENT state but conditions not met (no currentEvent, or not in EVENT room, or map/player null).");
                setState(GameState::MAP); 
            }
            break;
        }
        case GameState::GAME_OVER: {
            LOG_DEBUG("game", "Showing game over (switched on newState)");
            if (currentCombat_) {
                LOG_INFO("game", "Cleaning up combat state before game over");
                currentCombat_.reset();
            }
            transitioningFromCombat_ = false;
            int finalScore = calculateScore();
            LOG_INFO("game", "Game over - Final score: " + std::to_string(finalScore));
            if (ui_) ui_->showGameOver(map_ && map_->isBossDefeated(), finalScore);
            LOG_DEBUG("game", "Game over shown, awaiting transition to main menu");
            
            LOG_INFO("game", "Scheduling automatic transition to main menu in 5 seconds");
            std::thread([this]() {
                std::this_thread::sleep_for(std::chrono::seconds(5));
                LOG_INFO("game", "Auto-executing game over handler to return to main menu");
                currentCombat_.reset();
                transitioningFromCombat_ = false;
                currentEvent_.reset();
                map_.reset();
                player_.reset();
                setState(GameState::MAIN_MENU);
                LOG_INFO("game", "Auto-returned to main menu after game over");
            }).detach();
            break;
        }
        case GameState::REWARD: {
            LOG_DEBUG("game", "Showing combat rewards (switched on newState)");
            if (!ui_) {
                 LOG_WARNING("game", "UI is null in REWARD state. Rewards might not have been displayed if endCombat didn't show them.");
            }
            break;
        }
        default: {
            LOG_ERROR("game", "Unhandled game state in setState (switched on newState): " + std::to_string(static_cast<int>(newState)) + ". Current state_ is: " + std::to_string(static_cast<int>(state_)) + ". Previous state was: " + std::to_string(static_cast<int>(previousState)));
            break;
        }
    }
}

bool Game::createPlayer(const std::string& characterId, const std::string& playerName) {
    try {
        LOG_INFO("game", "Attempting to create player with character ID: " + characterId);

        auto it = allCharacters_.find(characterId);
        if (it == allCharacters_.end()) {
            LOG_ERROR("game", "Character data not found for ID: " + characterId);
            return false;
        }
        
        const CharacterData& charData = it->second;
        LOG_INFO("game", "Found character data for: " + charData.name);

        std::string nameToUse = playerName.empty() ? charData.name : playerName;
        
        LOG_INFO("game", "Creating player: " + nameToUse + " (ID: " + characterId +
                ", health: " + std::to_string(charData.max_health) +
                ", energy: " + std::to_string(charData.base_energy) + ")");
        player_ = std::make_unique<Player>(
            characterId,
            nameToUse,
            charData.max_health,
            charData.base_energy,
            charData.initial_hand_size
        );

        LOG_INFO("game", "Player created with initial hand size: " + std::to_string(charData.initial_hand_size));

        int cardsAdded = 0;
        if (!charData.starting_deck.empty()) {
            LOG_INFO("game", "Adding starting deck with " + std::to_string(charData.starting_deck.size()) + " cards");
            for (const auto& cardId : charData.starting_deck) {
                LOG_INFO("game", "Trying to load card: " + cardId);
                auto card = loadCard(cardId);
                if (card) {
                    player_->addCard(card);
                    cardsAdded++;
                    LOG_INFO("game", "Added card to deck: " + cardId);
                } else {
                    LOG_ERROR("game", "Failed to load starting card: " + cardId);
                }
            }
            LOG_INFO("game", "Successfully added " + std::to_string(cardsAdded) + " cards to starting deck. Expected: " + std::to_string(charData.starting_deck.size()));
        } else {
            LOG_WARNING("game", "No starting deck specified for character: " + characterId);
        }

        if (!charData.starting_relics.empty()) {
            LOG_INFO("game", "Adding " + std::to_string(charData.starting_relics.size()) + " starting relics");
            for (const auto& relicId : charData.starting_relics) {
                LOG_DEBUG("game", "Trying to load relic: " + relicId);
                auto relic = loadRelic(relicId);
                if (relic) {
                    player_->addRelic(relic);
                    LOG_DEBUG("game", "Added starting relic: " + relicId);
                } else {
                    LOG_ERROR("game", "Failed to load starting relic: " + relicId);
                }
            }
        } else {
            LOG_WARNING("game", "No starting relics specified for character: " + characterId);
        }
        
        LOG_INFO("game", "Player " + nameToUse + " created successfully.");
        return true;

    } catch (const std::exception& e) {
        LOG_ERROR("game", "Error creating player with ID " + characterId + ": " + std::string(e.what()));
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
    
    currentCombat_ = std::make_unique<Combat>(player_.get());
    
    currentCombat_->setGame(this);
    
    for (const auto& enemyId : enemies) {
        auto enemy = loadEnemy(enemyId);
        if (enemy) {
            currentCombat_->addEnemy(enemy);
        }
    }
    
    player_->beginCombat();
    currentCombat_->start();
    
    setState(GameState::COMBAT);
    
    return true;
}

void Game::endCombat(bool victorious) {
    if (!currentCombat_) {
        LOG_ERROR("game", "No active combat to end");
        return;
    }
    
    LOG_INFO("game", "Ending combat. Victory: " + std::string(victorious ? "true" : "false"));
    
    transitioningFromCombat_ = true;
    
    try {
        try {
            player_->endCombat();
        } catch (const std::exception& e) {
            LOG_ERROR("game", "Exception in player->endCombat: " + std::string(e.what()));
        }
        
        if (currentCombat_) {
            try {
                currentCombat_->end(victorious);
            } catch (const std::exception& e) {
                LOG_ERROR("game", "Exception in combat->end: " + std::string(e.what()));
            }
        }
        
        if (victorious) {
            int goldReward = 0;
            if (currentCombat_) {
                for (const auto& enemy : currentCombat_->getEnemies()) {
                    if (enemy) {
                        try {
                            goldReward += enemy->rollGoldReward();
                        } catch (const std::exception& e) {
                            LOG_ERROR("game", "Exception rolling gold reward: " + std::string(e.what()));
                        }
                    }
                }
            }
            
            LOG_INFO("game", "Combat victory! Gold reward: " + std::to_string(goldReward));
            
            player_->addGold(goldReward);
            
            if (map_) {
                try {
                    map_->markCurrentRoomVisited();
                    
                    const Room* currentRoom = map_->getCurrentRoom();
                    if (currentRoom && currentRoom->type == RoomType::BOSS) {
                        LOG_INFO("game", "Boss defeated! Generating next act");
                        
                        map_->markBossDefeated();
                        
                        int nextAct = map_->getAct() + 1;
                        if (generateMap(nextAct)) {
                            LOG_INFO("game", "Generated new map for act " + std::to_string(nextAct));
                        } else {
                            LOG_ERROR("game", "Failed to generate new map for act " + std::to_string(nextAct));
                            currentCombat_.reset();
                            transitioningFromCombat_ = false;
                            setState(GameState::GAME_OVER);
                            return;
                        }
                    }
                } catch (const std::exception& e) {
                    LOG_ERROR("game", "Exception processing map after combat: " + std::string(e.what()));
                }
            }
            
            LOG_INFO("game", "Showing rewards screen. Gold earned: " + std::to_string(goldReward));
            
            if (ui_) {
                try {
                    ui_->showRewards(goldReward, {}, {});
                } catch (const std::exception& e) {
                    LOG_ERROR("game", "Exception showing rewards: " + std::string(e.what()));
                }
            }
            
            LOG_INFO("game", "Clearing combat state");
            currentCombat_.reset();
            
            transitioningFromCombat_ = false;
            
            LOG_INFO("game", "Transitioning to map view");
            setState(GameState::MAP);
        } else {
            LOG_INFO("game", "Player defeated! Transitioning to game over screen");
            
            int finalScore = calculateScore();
            LOG_INFO("game", "Final score: " + std::to_string(finalScore));
            
            currentCombat_.reset();
            
            transitioningFromCombat_ = false;
            
            setState(GameState::GAME_OVER);
            LOG_INFO("game", "Transitioned to game over state");
        }
    } catch (const std::exception& e) {
        LOG_ERROR("game", "Exception in endCombat: " + std::string(e.what()));
        
        currentCombat_.reset();
        transitioningFromCombat_ = false;
        
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
    auto it = allCards_.find(id);
    if (it != allCards_.end()) {
        return it->second->cloneCard();
    }
    LOG_ERROR("game", "Card template not found for ID: " + id + ". Ensure loadAllCards() was called and the card ID is correct.");
    return nullptr;
}

bool Game::loadAllCards() {
    failedLoads_ = 0;
    try {
        std::string data_prefix = get_data_path_prefix();
        std::string cards_dir_path = data_prefix + "data/cards";
        LOG_DEBUG("game", "Loading all cards from directory: " + cards_dir_path);
        if (!fs::exists(cards_dir_path) || !fs::is_directory(cards_dir_path)) {
            LOG_ERROR("game", "Cards directory not found or is not a directory: " + cards_dir_path);
            return false;
        }
        for (const auto& entry : fs::directory_iterator(cards_dir_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string id_from_filename = entry.path().stem().string();
                LOG_DEBUG("game", "Attempting to load card '" + id_from_filename + "' from: " + entry.path().string());
                std::ifstream file(entry.path());
        if (!file.is_open()) {
                    LOG_ERROR("game", "Could not open card file: " + entry.path().string());
                    failedLoads_++;
                    continue;
        }
                json cardJson;
                file >> cardJson;
                file.close();
        
        auto card = std::make_shared<Card>();
                if (!card->loadFromJson(cardJson)) {
                    LOG_ERROR("game", "Failed to load card data from JSON for file: " + entry.path().string());
                    failedLoads_++;
                    continue;
        }

                allCards_[id_from_filename] = card;
    }
}
        LOG_INFO("game", "Loaded " + std::to_string(allCards_.size()) + " cards. " + std::to_string(failedLoads_) + " failed.");
        return failedLoads_ == 0;
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("game", "Filesystem error while loading cards: " + std::string(e.what()));
        return false;
    } catch (const json::exception& e) {
        LOG_ERROR("game", "JSON parsing error while loading cards: " + std::string(e.what()));
        return false;
    }
}

std::shared_ptr<Enemy> Game::loadEnemy(const std::string& id) {
    auto it = allEnemies_.find(id);
    if (it != allEnemies_.end()) {
        return it->second->cloneEnemy();
    }
    LOG_ERROR("game", "Enemy template not found for ID: " + id + ". Ensure loadAllEnemies() was called and the enemy ID is correct.");
            return nullptr;
        }
        
bool Game::loadAllEnemies() {
    failedLoads_ = 0;
    try {
        std::string data_prefix = get_data_path_prefix();
        std::string enemies_dir_path = data_prefix + "data/enemies";
        LOG_DEBUG("game", "Loading all enemies from directory: " + enemies_dir_path);

        if (!fs::exists(enemies_dir_path) || !fs::is_directory(enemies_dir_path)) {
            LOG_ERROR("game", "Enemies directory not found or is not a directory: " + enemies_dir_path);
            return false;
    }

        for (const auto& entry : fs::directory_iterator(enemies_dir_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string id_from_filename = entry.path().stem().string();
                LOG_DEBUG("game", "Attempting to load enemy '" + id_from_filename + "' from: " + entry.path().string());
                std::ifstream file(entry.path());
                if (!file.is_open()) {
                    LOG_ERROR("game", "Error: Could not open enemy file: " + entry.path().string());
                    failedLoads_++;
                    continue;
                }
                json enemyJson;
                file >> enemyJson;
                file.close();

                auto enemy = std::make_shared<Enemy>();
                if (!enemy->loadFromJson(enemyJson)) {
                    LOG_ERROR("game", "Failed to load enemy data from JSON for file: " + entry.path().string());
                    failedLoads_++;
                    continue;
                }
                allEnemies_[id_from_filename] = enemy;
                }
            }
        LOG_INFO("game", "Loaded " + std::to_string(allEnemies_.size()) + " enemies. " + std::to_string(failedLoads_) + " failed.");
        return failedLoads_ == 0;
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("game", "Filesystem error while loading enemies: " + std::string(e.what()));
        return false;
    } catch (const json::exception& e) {
        LOG_ERROR("game", "JSON parsing error while loading enemies: " + std::string(e.what()));
        return false;
    }
}

std::shared_ptr<Relic> Game::loadRelic(const std::string& id) {
    auto it = allRelics_.find(id);
    if (it != allRelics_.end()) {
        return it->second->cloneRelic();
    }
    LOG_ERROR("game", "Relic template not found for ID: " + id + ". Ensure loadAllRelics() was called and the relic ID is correct.");
            return nullptr;
}

bool Game::loadAllRelics() {
    failedLoads_ = 0;
    try {
        std::string data_prefix = get_data_path_prefix();
        std::string relics_dir_path = data_prefix + "data/relics";
        LOG_DEBUG("game", "Loading all relics from directory: " + relics_dir_path);
        if (!fs::exists(relics_dir_path) || !fs::is_directory(relics_dir_path)) {
            LOG_ERROR("game", "Relics directory not found or is not a directory: " + relics_dir_path);
            return false;
        }
        for (const auto& entry : fs::directory_iterator(relics_dir_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string id_from_filename = entry.path().stem().string();
                LOG_DEBUG("game", "Attempting to load relic '" + id_from_filename + "' from: " + entry.path().string());
                std::ifstream file(entry.path());
                if (!file.is_open()) {
                    LOG_ERROR("game", "Could not open relic file: " + entry.path().string());
                    failedLoads_++;
                    continue;
                }
                json relicJson;
                file >> relicJson;
                file.close();

                auto relic = std::make_shared<Relic>();
                if (!relic->loadFromJson(relicJson)) {
                    LOG_ERROR("game", "Failed to load relic data from JSON for file: " + entry.path().string());
                    failedLoads_++;
                    continue;
                }
                allRelics_[id_from_filename] = relic;
                }
            }
        LOG_INFO("game", "Loaded " + std::to_string(allRelics_.size()) + " relics. " + std::to_string(failedLoads_) + " failed.");
        return failedLoads_ == 0;
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("game", "Filesystem error while loading relics: " + std::string(e.what()));
        return false;
    } catch (const json::exception& e) {
        LOG_ERROR("game", "JSON parsing error while loading relics: " + std::string(e.what()));
        return false;
    }
}

UIInterface* Game::getUI() const {
    return ui_.get();
}

bool Game::processInput(const std::string& input) {
    if (!running_ && state_ != GameState::MAIN_MENU && state_ != GameState::CHARACTER_SELECT) {
        LOG_WARNING("game", "Input processed while game not actively running (state: " + std::to_string(static_cast<int>(state_)) + ")");
        if (state_ != GameState::MAIN_MENU && state_ != GameState::CHARACTER_SELECT) {
        return false;
    }
    }

    bool result = false;
    switch (state_) {
        case GameState::MAIN_MENU:
            result = handleMainMenuInput(input);
            break;
        case GameState::CHARACTER_SELECT:
            result = handleCharacterSelectInput(input);
            break;
        case GameState::MAP:
            result = handleMapInput(input);
            break;
        case GameState::COMBAT:
            result = handleCombatInput(input);
            break;
        case GameState::EVENT:
            result = handleEventInput(input);
            break;
        case GameState::REWARD:
            LOG_INFO("game", "Input received in REWARD state: " + input + ". Transitioning to MAP.");
            currentCombat_.reset();
            setState(GameState::MAP);
            result = true;
            break;
        case GameState::GAME_OVER:
            LOG_INFO("game", "Input received in GAME_OVER state: " + input + ". Transitioning to MAIN_MENU.");
            setState(GameState::MAIN_MENU);
            result = true;
            break;
        case GameState::SHOP:
            result = handleShopInput(input);
            break;
        default:
            LOG_ERROR("game", "Unhandled game state in processInput: " + std::to_string(static_cast<int>(state_)));
            result = false;
            break;
    }
    return result;
}

void Game::initializeInputHandlers() {
    inputHandlers_[GameState::MAIN_MENU] = [this](const std::string& input) {
        return handleMainMenuInput(input);
    };
    
    inputHandlers_[GameState::CHARACTER_SELECT] = [this](const std::string& input) {
        LOG_DEBUG("game", "Character selection handler received input: '" + input + "'");
        
        std::vector<std::string> availableCharacterIds;
        std::vector<std::string> availableCharacterNames;
        for (const auto& pair : allCharacters_) {
            availableCharacterIds.push_back(pair.first); // Store ID (e.g., "ironclad")
            availableCharacterNames.push_back(pair.second.name); // Store display name (e.g., "The Ironclad")
        }

        if (availableCharacterIds.empty()) {
            LOG_ERROR("game", "No characters loaded. Cannot proceed with character selection.");
            ui_->showMessage("Error: No characters available. Returning to main menu.", true);
            setState(GameState::MAIN_MENU);
            return false;
        }

        try {
            int choice = std::stoi(input);
            LOG_DEBUG("game", "Parsed choice: " + std::to_string(choice));
            
            if (choice >= 1 && choice <= static_cast<int>(availableCharacterIds.size())) {
                std::string selectedCharacterId = availableCharacterIds[choice - 1];
                LOG_DEBUG("game", "Selected character ID: " + selectedCharacterId);

                if (createPlayer(selectedCharacterId, "")) {
                    LOG_DEBUG("game", "Player '" + selectedCharacterId + "' created, generating map");
                        if (generateMap(1)) {
                            LOG_DEBUG("game", "Map generated, setting state to MAP");
                            setState(GameState::MAP);
                        return true;
                        } 
                        else {
                        LOG_ERROR("game", "Failed to generate map after creating player " + selectedCharacterId);
                        
                        player_.reset();
                            setState(GameState::MAIN_MENU);

                        return false;
                        }
                    } else {
                    LOG_ERROR("game", "Failed to create player with ID: " + selectedCharacterId);
                            setState(GameState::MAIN_MENU);
                    return false;
                        }
                    } else {
                LOG_DEBUG("game", "Invalid character choice number: " + std::to_string(choice));
                ui_->showMessage("Invalid choice. Please select a valid character number.", true);
                ui_->showCharacterSelection(availableCharacterNames);
                return false;
            }
        } catch (const std::exception& e) {
            LOG_ERROR("game", "Exception in character selection handler: " + std::string(e.what()) + " for input: '" + input + "'");
            ui_->showMessage("Invalid input. Please enter a number.", true);
            ui_->showCharacterSelection(availableCharacterNames);
            return false;
        }
    };
    
    inputHandlers_[GameState::MAP] = [this](const std::string& input) {
        return handleMapInput(input);
    };
    
    inputHandlers_[GameState::COMBAT] = [this](const std::string& input) {
        return handleCombatInput(input);
    };
    
    inputHandlers_[GameState::EVENT] = [this](const std::string& input) {
        return handleEventInput(input);
    };
    
    inputHandlers_[GameState::GAME_OVER] = [this](const std::string& input) {
        LOG_INFO("game", "Game over handler activated with input: '" + input + "'");
        
        int score = calculateScore();
        LOG_INFO("game", "Final score: " + std::to_string(score));
        
        LOG_INFO("game", "Cleaning up game state after game over");
        currentCombat_.reset();
        transitioningFromCombat_ = false;
        currentEvent_.reset();
        
        LOG_INFO("game", "Resetting map and player for new game");
        map_.reset();
        player_.reset();
        
        LOG_INFO("game", "Transitioning to main menu after game over");
        setState(GameState::MAIN_MENU);
        
        LOG_INFO("game", "Returned to main menu after game over");
        return true;
    };
    
    inputHandlers_[GameState::REWARD] = [this](const std::string&) {
        LOG_INFO("game", "Leaving rewards screen, returning to map");
        
        currentCombat_.reset();
        
        transitioningFromCombat_ = false;
        
        setState(GameState::MAP);
        return true;
    };
    
    inputHandlers_[GameState::SHOP] = [this](const std::string& input) {
        return handleShopInput(input);
    };
}

bool Game::handleMainMenuInput(const std::string& input) {
    LOG_INFO("game", "handleMainMenuInput received: '" + input + "'");
    try {
        std::string trimmed_input = input;
        trimmed_input.erase(trimmed_input.begin(), std::find_if(trimmed_input.begin(), trimmed_input.end(), [](unsigned char ch) {
            return !std::isspace(ch);
        }));
        trimmed_input.erase(std::find_if(trimmed_input.rbegin(), trimmed_input.rend(), [](unsigned char ch) {
            return !std::isspace(ch);
        }).base(), trimmed_input.end());

        LOG_INFO("game", "Trimmed input: '" + trimmed_input + "'");

        if (trimmed_input.empty()) {
            LOG_INFO("game", "Trimmed input is empty. Invalid choice.");
            ui_->showMessage("Invalid choice. Please enter a number (1-2" + std::string(developerModeEnabled_ ? " or 3" : "") + ").", true);
            ui_->showMainMenu();
            return false;
        }

        int choice = std::stoi(trimmed_input);
        LOG_INFO("game", "Parsed choice: " + std::to_string(choice));

        switch (choice) {
            case 1:
                LOG_INFO("game", "Choice is 1 (New Game), setting state to CHARACTER_SELECT");
                setState(GameState::CHARACTER_SELECT);
                LOG_INFO("game", "State after setState(CHARACTER_SELECT): " + std::to_string(static_cast<int>(state_)));
                return true;
            case 2:
                LOG_INFO("game", "Choice is 2 (Quit), calling shutdown()");
                shutdown();
                return true;
            case 3:
                if (developerMode_) {
                    developerModeEnabled_ = !developerModeEnabled_;
                    LOG_INFO("game", "Choice is 3. Toggled Developer Mode Enabled to: " + std::string(developerModeEnabled_ ? "ON" : "OFF"));
                    ui_->showMessage(std::string("Developer Mode Features now ") + (developerModeEnabled_ ? "ENABLED" : "DISABLED"), true);
                } else {
                     LOG_INFO("game", "Choice 3 selected, but base Developer Mode (config) is OFF. No action taken.");
                     ui_->showMessage("Developer options are not available.", true);
                }
                ui_->showMainMenu(); 
                return true;
            default:
                LOG_INFO("game", "Invalid choice number: " + std::to_string(choice) + ".");
                std::string valid_options_msg = "Invalid choice. Please enter 1 or 2";
                if (developerMode_) { 
                    valid_options_msg += " or 3 (Toggle Dev Features)";
                }
                valid_options_msg += ".";
                ui_->showMessage(valid_options_msg, true);
                ui_->showMainMenu();
                return false;
        }
    } catch (const std::invalid_argument& ia) {
        LOG_ERROR("game", "Exception in handleMainMenuInput (std::invalid_argument): '" + std::string(ia.what()) + "' for input: '" + input + "'");
        ui_->showMessage("Invalid input. Please enter a number.", true);
        ui_->showMainMenu();
        return false;
    } catch (const std::out_of_range& oor) {
        LOG_ERROR("game", "Exception in handleMainMenuInput (std::out_of_range): '" + std::string(oor.what()) + "' for input: '" + input + "'");
        ui_->showMessage("Invalid input. Number is too large.", true);
        ui_->showMainMenu();
        return false;
    }
    LOG_ERROR("game", "handleMainMenuInput reached end unexpectedly for input: '" + input + "'");
    return false;
}

bool Game::handleCharacterSelectInput(const std::string& input) {
    LOG_INFO("game", "handleCharacterSelectInput received: '" + input + "'. Delegating to registered handler.");
    
    auto it = inputHandlers_.find(GameState::CHARACTER_SELECT);
    if (it != inputHandlers_.end()) {
        return it->second(input);
    } else {
        LOG_ERROR("game", "No input handler found for GameState::CHARACTER_SELECT in handleCharacterSelectInput.");
        ui_->showMessage("Error: Character selection handler not available.", true);
        setState(GameState::MAIN_MENU);
        return false;
    }
}

bool Game::handleMapInput(const std::string& input) {
    if (!map_) {
        return false;
    }
    
    if (input.empty()) {
        LOG_INFO("game", "Empty input in map view, redisplaying map");
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
        bool isValidNumber = !input.empty() && std::all_of(input.begin(), input.end(), 
            [](char c) { return std::isdigit(c); });
            
        if (!isValidNumber) {
            LOG_INFO("game", "Invalid map input: '" + input + "' (not a number)");
            int currentRoomId = -1;
            if (map_->getCurrentRoom()) {
                currentRoomId = map_->getCurrentRoom()->id;
            }
            ui_->showMessage("Please enter a valid room number.", true);
            ui_->showMap(currentRoomId, map_->getAvailableRooms(), map_->getAllRooms());
            return true;
        }
        
        int selectedIndex = std::stoi(input) - 1;
        
        std::vector<int> availableRooms = map_->getAvailableRooms();
        
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
        
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(availableRooms.size())) {
            int roomId = availableRooms[selectedIndex];
            
            if (map_->canMoveTo(roomId)) {
                if (map_->moveToRoom(roomId)) {
                    map_->markCurrentRoomVisited();
                    
                    const Room* room = map_->getCurrentRoom();
                    if (room) {
                        switch (room->type) {
                            case RoomType::MONSTER: {
                                std::vector<std::string> availableEnemies;
                                int floorRange = map_->getEnemyFloorRange();
                                
                                LOG_INFO("game", "Selecting enemy for monster room at floor range: " + std::to_string(floorRange));
                                
                                try {
                                    for (const auto& entry : fs::directory_iterator("data/enemies")) {
                                        if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                            std::string id = entry.path().stem().string();
                                            json enemyData;
                                            std::ifstream file(entry.path());
                                            if (file.is_open()) {
                                                file >> enemyData;
                                                
                                                bool isAppropriate = true;
                                                
                                                if ((enemyData.contains("is_elite") && enemyData["is_elite"].get<bool>()) || 
                                                    (enemyData.contains("is_boss") && enemyData["is_boss"].get<bool>())) {
                                                    isAppropriate = false;
                                                }
                                                
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
                                                    
                                                    bool isAppropriate = true;
                                                    
                                                    if (!enemyData.contains("is_elite") || !enemyData["is_elite"].get<bool>()) {
                                                        isAppropriate = false;
                                                    }
                                                    
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
                                
                                if (availableElites.empty()) {
                                    LOG_WARNING("game", "No elite enemies found, falling back to multiple basic enemies");
                                    
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
                                        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                                        std::mt19937 gen(seed);
                                        std::vector<std::string> encounter;
                                        
                                        std::uniform_int_distribution<> dist(0, basicEnemies.size() - 1);
                                        int firstEnemyIndex = dist(gen);
                                        encounter.push_back(basicEnemies[firstEnemyIndex]);
                                        
                                        if (basicEnemies.size() > 1) {
                                            std::vector<std::string> remainingEnemies = basicEnemies;
                                            remainingEnemies.erase(remainingEnemies.begin() + firstEnemyIndex);
                                            std::uniform_int_distribution<> dist2(0, remainingEnemies.size() - 1);
                                            encounter.push_back(remainingEnemies[dist2(gen)]);
                                        } else {
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
                                std::vector<std::string> bossEnemies;
                                try {
                                    for (const auto& entry : fs::directory_iterator("data/enemies")) {
                                        if (entry.is_regular_file() && entry.path().extension() == ".json") {
                                            try {
                                                std::string id = entry.path().stem().string();
                                                json enemyData;
                                                std::ifstream file(entry.path());
                                                if (file.is_open()) {
                                                    file >> enemyData;
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
                                
                                unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                                std::mt19937 gen(seed);
                                std::uniform_int_distribution<> dist(0, bossEnemies.size() - 1);
                                int enemyIndex = dist(gen);
                                startCombat({bossEnemies[enemyIndex]});
                                break;
                            }
                            case RoomType::EVENT: {
                                if (allEvents_.empty()) {
                                    ui_->showMessage("Error: No events found.", true);
                                    ui_->showMap(roomId, map_->getAvailableRooms(), map_->getAllRooms());
                                    return true;
                                }
                                
                                std::vector<std::string> eventIds;
                                for (const auto& [id, event] : allEvents_) {
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
                                startEvent("rest_site");
                                break;
                            }
                            case RoomType::TREASURE: {
                                LOG_INFO("game", "Player entered TREASURE room #" + std::to_string(room->id));
                                if (player_) {
                                    int goldAmount = 50 + (rand() % 51);
                                    player_->addGold(goldAmount);
                                    ui_->showMessage("You found a treasure chest containing " + std::to_string(goldAmount) + " gold!", true);

                                    if (!allRelics_.empty()) {
                                        std::vector<std::string> relicIds;
                                        for(const auto& pair : allRelics_) {
                                            relicIds.push_back(pair.first);
                                        }
                                        std::string randomRelicId = relicIds[rand() % relicIds.size()]; 
                                        auto relic = loadRelic(randomRelicId);
                                        if (relic) {
                                            player_->addRelic(relic);
                                            ui_->showMessage("You also found a relic: " + relic->getName(), true);
                                        } else {
                                            LOG_ERROR("game", "Failed to load random relic '" + randomRelicId + "' for treasure room.");
                                        }
                                    } else {
                                        LOG_WARNING("game", "No relics loaded, cannot give random relic in treasure room.");
                                    }
                                    setState(GameState::MAP); 
                                } else {
                                    LOG_ERROR("game", "Player is null in TREASURE room.");
                                    setState(GameState::MAP);
                                }
                                break;
                            }
                            case RoomType::SHOP: {
                                LOG_INFO("game", "Player entered SHOP room #" + std::to_string(room->id));
                                std::vector<Card*> cardsForSale; 
                                std::vector<Relic*> relicsForSale;
                                if (allCards_.size() >= 3) {
                                    auto it = allCards_.begin();
                                    std::advance(it, rand() % (allCards_.size() - 2));
                                    cardsForSale.push_back(it->second.get()); it++;
                                    cardsForSale.push_back(it->second.get()); it++;
                                    cardsForSale.push_back(it->second.get());
                                }
                                if (!allRelics_.empty()) {
                                    auto it = allRelics_.begin();
                                    std::advance(it, rand() % allRelics_.size());
                                    relicsForSale.push_back(it->second.get());
                                }

                                if (player_) {
                                    ui_->showShop(cardsForSale, relicsForSale, shopRelicPrices_, player_->getGold());
                                    setState(GameState::SHOP);
                                } else {
                                    LOG_ERROR("game", "Player is null when trying to enter SHOP room.");
                                    setState(GameState::MAP);
                                }
                                break;
                            }
                            default:
                                int currentRoomId = room->id;
                                ui_->showMap(currentRoomId, map_->getAvailableRooms(), map_->getAllRooms());
                                break;
                        }
                        
                        return true;
                    }
                }
            }
        }
        
        int currentRoomId = -1;
        if (map_->getCurrentRoom()) {
            currentRoomId = map_->getCurrentRoom()->id;
        }
        
        ui_->showMessage("Cannot move to that room.", true);
        ui_->showMap(currentRoomId, map_->getAvailableRooms(), map_->getAllRooms());
    } catch (const std::exception&) {
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
    
    if (transitioningFromCombat_) {
        LOG_DEBUG("game", "Already transitioning from combat, skipping input processing");
        return true;
    }
    
    if (currentCombat_->isCombatOver()) {
        LOG_INFO("game", "Combat is over, transitioning. Player defeated: " + 
            std::string(currentCombat_->isPlayerDefeated() ? "true" : "false"));
            
        endCombat(!currentCombat_->isPlayerDefeated());
        return true;
    }
    
    if (awaitingEnemySelection_) {
        if (input == "cancel" || input == "c") {
            awaitingEnemySelection_ = false;
            selectedCardIndex_ = -1;
            ui_->showCombat(currentCombat_.get());
            return true;
        }
        
        try {
            int targetIndex = std::stoi(input) - 1;
            
            const auto& hand = player_->getHand();
            if (selectedCardIndex_ >= 0 && selectedCardIndex_ < static_cast<int>(hand.size())) {
                const std::string& cardName = hand[selectedCardIndex_]->getName();
                
                if (currentCombat_->playCard(selectedCardIndex_, targetIndex)) {
                    awaitingEnemySelection_ = false;
                    selectedCardIndex_ = -1;
                    ui_->showCombat(currentCombat_.get());
                } else {
                    ui_->showMessage("Cannot target that enemy with " + cardName + ". Try a different target or card.", true);
                    ui_->showEnemySelectionMenu(currentCombat_.get(), cardName);
                }
            } else {
                awaitingEnemySelection_ = false;
                selectedCardIndex_ = -1;
                ui_->showCombat(currentCombat_.get());
            }
        } catch (const std::exception&) {
            ui_->showMessage("Invalid target. Please enter a valid enemy number or 'cancel'.", true);
            const auto& hand = player_->getHand();
            if (selectedCardIndex_ >= 0 && selectedCardIndex_ < static_cast<int>(hand.size())) {
                ui_->showEnemySelectionMenu(currentCombat_.get(), hand[selectedCardIndex_]->getName());
            } else {
                awaitingEnemySelection_ = false;
                selectedCardIndex_ = -1;
                ui_->showCombat(currentCombat_.get());
            }
        }
        
        return true;
    }
    
    if (input == "end" || input == "e") {
        currentCombat_->endPlayerTurn();
        ui_->showCombat(currentCombat_.get());
    } else if (input == "help" || input == "h") {
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
        try {
            size_t spacePos = input.find(' ');
            std::string cardStr = input.substr(0, spacePos);
            
            int cardIndex = std::stoi(cardStr) - 1;
            
            const auto& hand = player_->getHand();
            if (cardIndex < 0 || cardIndex >= static_cast<int>(hand.size())) {
                ui_->showMessage("Invalid card number. Please enter a number between 1 and " + 
                                std::to_string(hand.size()) + ".", true);
                ui_->showCombat(currentCombat_.get());
                return true;
            }
            
            if (spacePos != std::string::npos) {
                int targetIndex = std::stoi(input.substr(spacePos + 1)) - 1;
                
                if (currentCombat_->playCard(cardIndex, targetIndex)) {
                    ui_->showCombat(currentCombat_.get());
                } else {
                    ui_->showMessage("Cannot play that card on that target. Check energy cost or valid targets.", true);
                    ui_->showCombat(currentCombat_.get());
                }
            } else {
                Card* card = hand[cardIndex].get();
                
                if (card && card->needsTarget()) {
                    if (currentCombat_->getEnemyCount() == 1 && currentCombat_->getEnemy(0) && currentCombat_->getEnemy(0)->isAlive()) {
                        if (currentCombat_->playCard(cardIndex, 0)) {
                            ui_->showCombat(currentCombat_.get());
                        } else {
                            ui_->showMessage("Cannot play " + card->getName() + " on the enemy. Check energy cost.", true);
                            ui_->showCombat(currentCombat_.get());
                        }
                    } else {
                        awaitingEnemySelection_ = true;
                        selectedCardIndex_ = cardIndex;
                        ui_->showEnemySelectionMenu(currentCombat_.get(), card->getName());
                    }
                } else {
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
    auto it = allEvents_.find(id);
    if (it != allEvents_.end()) {
        std::unique_ptr<Entity> clonedEntity = it->second->clone();
        Event* clonedEventRawPtr = dynamic_cast<Event*>(clonedEntity.get());
        if (clonedEventRawPtr) {
            clonedEntity.release();
            return std::shared_ptr<Event>(clonedEventRawPtr);
        } else {
            LOG_ERROR("game", "Failed to cast cloned Entity to Event for ID: " + id);
            return nullptr;
        }
    }
    LOG_ERROR("game", "Event template not found for ID: " + id + ". Ensure loadAllEvents() was called and the event ID is correct.");
    return nullptr;
}

bool Game::loadAllEvents() {
    failedLoads_ = 0;
    try {
        std::string data_prefix = get_data_path_prefix();
        std::string events_dir_path = data_prefix + "data/events";
        LOG_DEBUG("game", "Loading all events from directory: " + events_dir_path);
        if (!fs::exists(events_dir_path) || !fs::is_directory(events_dir_path)) {
            LOG_ERROR("game", "Events directory not found or is not a directory: " + events_dir_path);
            return false;
        }
        
        for (const auto& entry : fs::directory_iterator(events_dir_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string id_from_filename = entry.path().stem().string();
                LOG_DEBUG("game", "Attempting to load event '" + id_from_filename + "' from: " + entry.path().string());
                std::ifstream file(entry.path());
                if (!file.is_open()) {
                    LOG_ERROR("game", "Could not open event file: " + entry.path().string());
                    failedLoads_++;
                    continue;
                }
                json eventJson;
                file >> eventJson;
                file.close();

                auto event = std::make_shared<Event>();
                if (!event->loadFromJson(eventJson)) {
                    LOG_ERROR("game", "Failed to load event data from JSON for file: " + entry.path().string());
                    failedLoads_++;
                    continue;
                }
                allEvents_[id_from_filename] = event;
            }
        }
        LOG_INFO("game", "Loaded " + std::to_string(allEvents_.size()) + " events. " + std::to_string(failedLoads_) + " failed.");
        return failedLoads_ == 0;
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("game", "Filesystem error while loading events: " + std::string(e.what()));
        return false;
    } catch (const json::exception& e) {
        LOG_ERROR("game", "JSON parsing error while loading events: " + std::string(e.what()));
        return false;
    }
}

bool Game::startEvent(const std::string& eventId) {
    LOG_INFO("game", "Starting event: " + eventId);
    
    currentEvent_ = loadEvent(eventId);
    if (!currentEvent_) {
        LOG_ERROR("game", "Failed to load event: " + eventId);
        return false;
    }
    
    setState(GameState::EVENT);
    return true;
}

void Game::endEvent() {
    LOG_INFO("game", "Ending event");
    
    currentEvent_ = nullptr;
    
    if (map_) {
        map_->markCurrentRoomVisited();
    }
    
    setState(GameState::MAP);
}

bool Game::handleEventInput(const std::string& input) {
    if (!currentEvent_ || !player_) {
        LOG_ERROR("game", "Event or player is null in handleEventInput");
        setState(GameState::MAP);
        return false;
    }
    
    int choiceIndex = -1;
    try {
        if (!input.empty()) {
            choiceIndex = std::stoi(input) - 1;
        }
    } catch (const std::exception& e) {
        LOG_ERROR("game", "Invalid input for event choice: " + input);
        ui_->showMessage("Invalid choice. Please enter a number.", true);
        ui_->showEvent(currentEvent_.get(), player_.get());
            return true;
        }
        
    if (choiceIndex < 0 || choiceIndex >= static_cast<int>(currentEvent_->getAllChoices().size())) {
        LOG_WARNING("game", "Event choice index out of bounds: " + std::to_string(choiceIndex));
        ui_->showMessage("Invalid choice number.", true);
        ui_->showEvent(currentEvent_.get(), player_.get());
            return true;
        }
        
    const auto& choice = currentEvent_->getAllChoices()[choiceIndex];
    std::string resultText = choice.resultText;

    LOG_INFO("game", "Player chose event option " + std::to_string(choiceIndex + 1) + ": " + choice.text);
    LOG_INFO("game", "Base result text: " + resultText);


    bool combatStarted = false;
    for (const auto& effect : choice.effects) {
        LOG_DEBUG("game", "Processing effect: type=" + effect.type + ", value=" + std::to_string(effect.value) + ", target=" + effect.target);
        if (effect.type == "HP") {
            int actualHpChange = 0;
            if (effect.value > 0) {
                actualHpChange = player_->heal(effect.value);
                resultText += " You gained " + std::to_string(actualHpChange) + " HP.";
            } else if (effect.value < 0) {
                actualHpChange = player_->takeDamage(-effect.value);
                resultText += " You lost " + std::to_string(-effect.value) + " HP.";
            }
        } else if (effect.type == "GAIN_HEALTH") {
            if (effect.value > 0) {
                int actualHpChange = player_->heal(effect.value);
                resultText += " You gained " + std::to_string(actualHpChange) + " HP.";
                LOG_INFO("game", "Player healed for " + std::to_string(actualHpChange) + " HP from event.");
        } else {
                LOG_WARNING("game", "GAIN_HEALTH effect with non-positive value: " + std::to_string(effect.value));
            }
        } else if (effect.type == "MAX_HP" || effect.type == "INCREASE_MAX_HEALTH") {
            player_->setMaxHealth(player_->getMaxHealth() + effect.value);
            player_->heal(effect.value);
            resultText += " Your Max HP increased by " + std::to_string(effect.value) + ".";
        } else if (effect.type == "GOLD") {
            player_->addGold(effect.value);
            resultText += " You gained " + std::to_string(effect.value) + " gold.";
        } else if (effect.type == "CARD" || effect.type == "ADD_CARD") {
            auto card = loadCard(effect.target);
            if (card) {
                player_->addCardToDeck(card);
                resultText += " You obtained the card: " + card->getName() + ".";
            } else {
                resultText += " Failed to obtain card: " + effect.target + ".";
                LOG_ERROR("game", "Failed to load card " + effect.target + " from event effect.");
            }
        } else if (effect.type == "ADD_CARD_RANDOM") {
            if (!effect.target.empty() && effect.target != "ANY") {
            }
            
            std::shared_ptr<Card> card = this->getRandomCardFromMasterList(effect.target.empty() ? "ANY" : effect.target);
            if (card) {
                player_->addCardToDeck(card);
                resultText += " You obtained a random card: " + card->getName() + ".";
        } else {
                resultText += " Failed to obtain a random card with target/rarity: " + effect.target + ".";
            }
        } else if (effect.type == "REMOVE_CARD") {
            if (!player_) {
                resultText += " Error: Player not found for card removal.";
                LOG_ERROR("game", "Player object is null during REMOVE_CARD event effect.");
            } else if (effect.target.empty() || effect.target == "RANDOM") {
                resultText += " Random card removal is not yet fully implemented for this event/encounter.";
                LOG_WARNING("game", "Encounter REMOVE_CARD effect for RANDOM target is not fully implemented.");
            } else {
                std::string cardIdToRemove = effect.target;
                if (player_->removeCardFromDeck(cardIdToRemove, false)) {
                    resultText += " Card '" + cardIdToRemove + "' was removed from your deck.";
                    LOG_INFO("game", "Event REMOVE_CARD: Successfully removed '" + cardIdToRemove + "'.");
                } else {
                    resultText += " Could not remove card '" + cardIdToRemove + "' (not found or error).";
                    LOG_WARNING("game", "Event REMOVE_CARD: Failed to remove '" + cardIdToRemove + "'.");
                }
            }
        } else if (effect.type == "UPGRADE_CARD") {
            std::string upgrade_outcome = upgradeCard();
            if (upgrade_outcome.rfind("Error:", 0) == 0 || upgrade_outcome.rfind("Failed to upgrade", 0) == 0 || upgrade_outcome.rfind("No card upgraded", 0) == 0) {
                resultText += " " + upgrade_outcome;
            } else {
                resultText += " You upgraded " + upgrade_outcome + ".";
            }
            LOG_INFO("game", "Event UPGRADE_CARD effect processed. Outcome: " + upgrade_outcome);
        } else if (effect.type == "RELIC" || effect.type == "ADD_RELIC") {
            auto relic = loadRelic(effect.target);
            if (relic) {
                player_->addRelic(relic);
                resultText += " You obtained the relic: " + relic->getName() + ".";
            } else {
                resultText += " Failed to obtain relic: " + effect.target + ".";
                LOG_ERROR("game", "Failed to load relic " + effect.target + " from event effect.");
            }
        } else if (effect.type == "ADD_RELIC_RANDOM") {
            std::shared_ptr<Relic> relic = this->getRandomRelicFromMasterList(); 
            if (relic) {
                player_->addRelic(relic);
                resultText += " You obtained a random relic: " + relic->getName() + ".";
            } else {
                resultText += " Failed to obtain a random relic.";
            }
        } else if (effect.type == "START_COMBAT" || effect.type == "COMBAT") {
            std::vector<std::string> enemyIds;
            std::stringstream ss(effect.target);
            std::string segment;
            while(std::getline(ss, segment, ',')) {
               enemyIds.push_back(segment);
            }
            if (!enemyIds.empty()) {
                startCombat(enemyIds);
                combatStarted = true; 
            } else {
                resultText += " Failed to start combat: no enemies specified.";
                LOG_ERROR("game", "START_COMBAT effect with no enemy IDs in event " + currentEvent_->getId());
            }
        } else {
            LOG_WARNING("game", "Unknown event effect type: " + effect.type + " for event " + currentEvent_->getId());
        }
    }

    ui_->showEventResult(resultText);
    
    if (!combatStarted) {
        currentEvent_.reset();
        setState(GameState::MAP);
    } else {
        currentEvent_.reset(); 
    }
    
    return true;
}

bool Game::loadGameData() {
    std::string data_prefix = get_data_path_prefix();
    LOG_INFO("game", "Data path prefix: " + data_prefix);

    std::vector<std::string> dataDirs = {
        data_prefix + "data",
        data_prefix + "data/cards",
        data_prefix + "data/enemies",
        data_prefix + "data/relics",
        data_prefix + "data/characters",
        data_prefix + "data/events"
    };
    
    for (const auto& dir_path_str : dataDirs) {
        fs::path dir_path(dir_path_str);
        if (!fs::exists(dir_path)) {
            try {
                LOG_INFO("game", "Creating directory: " + dir_path.string());
                fs::create_directories(dir_path);
            } catch (const fs::filesystem_error& e) {
                LOG_ERROR("game", "Failed to create directory: " + dir_path.string() + ": " + e.what());
                return false;
            }
        } else if (!fs::is_directory(dir_path)) {
            LOG_ERROR("game", "Path exists but is not a directory: " + dir_path.string());
            return false;
        }
    }
    
    if (!loadAllCharacters()) {
        LOG_ERROR("game", "Failed to load characters.");
        return false;
    }

    if (!loadAllCards()) {
        LOG_ERROR("game", "Failed to load cards.");
        return false;
    }
    
    if (!loadAllEnemies()) {
        LOG_ERROR("game", "Failed to load enemies.");
        return false;
    }
    
    if (!loadAllRelics()) {
        LOG_ERROR("game", "Failed to load relics.");
        return false;
    }
    
    if (!loadAllEvents()) {
        LOG_ERROR("game", "Failed to load events.");
        return false;
    }
    
    LOG_INFO("game", "Game data loading complete.");
    return true;
}

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

std::string Game::upgradeCard() {
    if (!player_) {
        LOG_ERROR("game", "Cannot upgrade card: player is null");
        return "Error: Player not found.";
    }
    if (!ui_) {
        LOG_ERROR("game", "Cannot upgrade card: UI is null");
        return "Error: UI not available.";
    }

    std::vector<std::shared_ptr<Card>> allPlayerCards;
    const auto& drawPile = player_->getDrawPile();
    allPlayerCards.insert(allPlayerCards.end(), drawPile.begin(), drawPile.end());
    const auto& discardPile = player_->getDiscardPile();
    allPlayerCards.insert(allPlayerCards.end(), discardPile.begin(), discardPile.end());
    const auto& hand = player_->getHand();
    allPlayerCards.insert(allPlayerCards.end(), hand.begin(), hand.end());

    std::vector<std::shared_ptr<Card>> upgradableCards;
    for (const auto& card_ptr : allPlayerCards) {
        if (card_ptr && card_ptr->isUpgradable() && !card_ptr->isUpgraded()) {
            upgradableCards.push_back(card_ptr);
        }
    }

    if (upgradableCards.empty()) {
        LOG_INFO("game", "Player has no cards available to upgrade.");
        ui_->showMessage("You have no cards that can be upgraded.", true);
        return "No card upgraded (none available).";
    }

    std::vector<Card*> displayCards;
    for (const auto& card_ptr : upgradableCards) {
        displayCards.push_back(card_ptr.get());
    }

    ui_->showCards(displayCards, "Select a card to upgrade:", true); 
                                     
    std::string input_str;
    bool valid_input = false;
    int chosen_idx = -1;

    while(!valid_input) {
        input_str = ui_->getInput("Enter card number to upgrade (1-" + std::to_string(upgradableCards.size()) + "), or 'cancel': ");
        if (input_str == "cancel") {
            LOG_INFO("game", "Card upgrade cancelled by user.");
            ui_->showMessage("Upgrade cancelled.", true);
            return "No card upgraded (cancelled).";
        }
        try {
            chosen_idx = std::stoi(input_str) - 1;
            if (chosen_idx >= 0 && chosen_idx < static_cast<int>(upgradableCards.size())) {
                valid_input = true;
            } else {
                ui_->showMessage("Invalid selection. Please enter a number from the list or 'cancel'.", true);
            }
        } catch (const std::invalid_argument& ia) {
            ui_->showMessage("Invalid input. Please enter a number or 'cancel'.", true);
        } catch (const std::out_of_range& oor) {
            ui_->showMessage("Invalid input. Number is too large.", true);
        }
    }
    
    std::shared_ptr<Card> selectedCard = upgradableCards[chosen_idx];
    std::string originalCardName = selectedCard->getName();

    if (selectedCard->upgrade()) {
        std::string upgradedCardName = selectedCard->getName();
        LOG_INFO("game", "Player upgraded '" + originalCardName + "' to '" + upgradedCardName + "'.");
        ui_->showMessage("Upgraded " + originalCardName + " to " + upgradedCardName + "!", true);
        return upgradedCardName;
    } else {
        LOG_ERROR("game", "Failed to upgrade card: " + originalCardName + " (upgrade() returned false).");
        ui_->showMessage("Failed to upgrade " + originalCardName + ". It might not be upgradable or already upgraded.", true);
        return "Upgrade failed for " + originalCardName + ".";
    }
}

int Game::calculateScore() const {
    int score = 0;
    
    if (!player_) {
        return score;
    }
    
    score += player_->getGold();
    
    score += player_->getHealth() * 2;
    
    if (map_) {
        const Room* currentRoom = map_->getCurrentRoom();
        if (currentRoom) {
            score += currentRoom->y * 10;
        }
        
        score += map_->getAct() * 100;
        
        if (map_->isBossDefeated()) {
            score += 500;
        }
    }
    
    const auto& relics = player_->getRelics();
    score += relics.size() * 50;
    
    const auto& drawPile = player_->getDrawPile();
    const auto& discardPile = player_->getDiscardPile();
    const auto& hand = player_->getHand();
    int deckSize = drawPile.size() + discardPile.size() + hand.size();
    score += deckSize * 5;
    
    LOG_INFO("game", "Calculated final score: " + std::to_string(score));
    return score;
}

std::shared_ptr<Card> Game::getCardData(const std::string& id) const {
    auto it = allCards_.find(id);
    if (it != allCards_.end()) {
        return it->second;
    }
    LOG_WARNING("game", "Card data not found for ID: " + id);
    return nullptr;
}

std::shared_ptr<Enemy> Game::getEnemyData(const std::string& id) const {
    auto it = allEnemies_.find(id);
    if (it != allEnemies_.end()) {
        return it->second;
    }
    LOG_WARNING("game", "Enemy data not found for ID: " + id);
    return nullptr;
}

std::shared_ptr<Relic> Game::getRelicData(const std::string& id) const {
    auto it = allRelics_.find(id);
    if (it != allRelics_.end()) {
        return it->second;
    }
    LOG_WARNING("game", "Relic data not found for ID: " + id);
    return nullptr;
}

std::shared_ptr<Event> Game::getEventData(const std::string& id) const {
    auto it = allEvents_.find(id);
    if (it != allEvents_.end()) {
        return it->second;
    }
    LOG_WARNING("game", "Event data not found for ID: " + id);
    return nullptr;
}

bool Game::handleShopInput(const std::string& input) {
    if (!player_ || !ui_) {
        LOG_ERROR("game", "Player or UI is null in handleShopInput");
        setState(GameState::MAP);
        return false;
    }

    std::string trimmed_input = input;
    trimmed_input.erase(trimmed_input.begin(), std::find_if(trimmed_input.begin(), trimmed_input.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
    trimmed_input.erase(std::find_if(trimmed_input.rbegin(), trimmed_input.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), trimmed_input.end());
    
    std::transform(trimmed_input.begin(), trimmed_input.end(), trimmed_input.begin(),
                   [](unsigned char c){ return std::tolower(c); });

    if (trimmed_input == "leave" || trimmed_input == "l") {
        LOG_INFO("game", "Player chose to leave shop.");
        setState(GameState::MAP);
        return true;
    }

    if (trimmed_input.empty()) {
        ui_->showMessage("Please enter an item to buy (e.g., C1, R1) or 'leave'.", true);
        ui_->showShop(shopCardsForSale_, shopRelicsForSale_, shopRelicPrices_, player_->getGold());
        return true;
    }

    char item_type_char = trimmed_input[0];
    std::string item_number_str = trimmed_input.substr(1);
    int item_idx = -1;

    if ((item_type_char != 'c' && item_type_char != 'r') || item_number_str.empty()) {
        ui_->showMessage("Invalid format. Use C<number> for cards, R<number> for relics, or 'leave'.", true);
        ui_->showShop(shopCardsForSale_, shopRelicsForSale_, shopRelicPrices_, player_->getGold());
        return true;
    }

    try {
        item_idx = std::stoi(item_number_str) - 1;
    } catch (const std::exception& e) {
        ui_->showMessage("Invalid item number. Please use format C<number> or R<number>.", true);
        ui_->showShop(shopCardsForSale_, shopRelicsForSale_, shopRelicPrices_, player_->getGold());
        return true;
    }

    if (item_type_char == 'c') {
        LOG_DEBUG("game_shop", "Checking card purchase. item_idx: " + std::to_string(item_idx) + ", shopCardsForSale_.size(): " + std::to_string(shopCardsForSale_.size()));
        if (item_idx >= 0 && item_idx < static_cast<int>(shopCardsForSale_.size())) {
            Card* selectedCard = shopCardsForSale_[item_idx];
            int cardCost = selectedCard->getCost();
            
            if (player_->getGold() >= cardCost) {
                if (player_->spendGold(cardCost)) {
                    player_->addCardToDeck(std::make_shared<Card>(*selectedCard));
                    ui_->showMessage("You bought " + selectedCard->getName() + " for " + std::to_string(cardCost) + " gold!", true);
                    LOG_INFO("game", "Player bought card: " + selectedCard->getName());
                    shopCardsForSale_.erase(shopCardsForSale_.begin() + item_idx);
                    ui_->showShop(shopCardsForSale_, shopRelicsForSale_, shopRelicPrices_, player_->getGold()); 
                } else {
                     ui_->showMessage("Something went wrong with spending gold.", true);
                }
            } else {
                ui_->showMessage("You don't have enough gold to buy " + selectedCard->getName() + ".", true);
            }
        } else {
            ui_->showMessage("Invalid card number.", true);
        }
    } else if (item_type_char == 'r') {
        if (item_idx >= 0 && item_idx < static_cast<int>(shopRelicsForSale_.size())) {
            Relic* selectedRelic = shopRelicsForSale_[item_idx];
            
            int relicCost = 150;
            auto priceIt = shopRelicPrices_.find(selectedRelic);
            if (priceIt != shopRelicPrices_.end()) {
                relicCost = priceIt->second;
            } else {
                if (selectedRelic->getRarity() == RelicRarity::RARE) relicCost = 250;
                else if (selectedRelic->getRarity() == RelicRarity::UNCOMMON) relicCost = 200;
                else if (selectedRelic->getRarity() == RelicRarity::BOSS) relicCost = 300;
                else if (selectedRelic->getRarity() == RelicRarity::SHOP) relicCost = 120;
                LOG_WARNING("game_shop", "Relic price not found in map, calculated based on rarity instead: " + std::to_string(relicCost));
            }

            if (player_->getGold() >= relicCost) {
                if (player_->spendGold(relicCost)) {
                    player_->addRelic(std::make_shared<Relic>(*selectedRelic));
                    ui_->showMessage("You bought " + selectedRelic->getName() + " for " + std::to_string(relicCost) + " gold!", true);
                    LOG_INFO("game", "Player bought relic: " + selectedRelic->getName());

                    shopRelicPrices_.erase(selectedRelic);
                    shopRelicsForSale_.erase(shopRelicsForSale_.begin() + item_idx);
                    ui_->showShop(shopCardsForSale_, shopRelicsForSale_, shopRelicPrices_, player_->getGold());
                } else {
                     ui_->showMessage("Something went wrong with spending gold.", true);
                }
            } else {
                ui_->showMessage("You don't have enough gold to buy " + selectedRelic->getName() + ".", true);
            }
        } else {
            ui_->showMessage("Invalid relic number.", true);
        }
    } else {
        ui_->showMessage("Invalid item type. Please use C<number> or R<number>.", true);
    }
    
    if (state_ == GameState::SHOP) {
        ui_->showShop(shopCardsForSale_, shopRelicsForSale_, shopRelicPrices_, player_->getGold());
    }
    return true;
}

CardRarity stringToCardRarity(const std::string& rarity_str) {
    std::string upper_rarity = rarity_str;
    std::transform(upper_rarity.begin(), upper_rarity.end(), upper_rarity.begin(), ::toupper);

    if (upper_rarity == "COMMON") return CardRarity::COMMON;
    if (upper_rarity == "UNCOMMON") return CardRarity::UNCOMMON;
    if (upper_rarity == "RARE") return CardRarity::RARE;
    if (upper_rarity == "SPECIAL") return CardRarity::SPECIAL;
    if (upper_rarity == "BASIC") return CardRarity::BASIC;
    return CardRarity::COMMON;
}

std::shared_ptr<Card> Game::getRandomCardFromMasterList(const std::string& rarity_filter_str) {
    std::vector<std::shared_ptr<Card>> eligibleCards;
    bool any_rarity = (rarity_filter_str == "ANY" || rarity_filter_str.empty());
    CardRarity target_rarity = CardRarity::COMMON;
    if (!any_rarity) {
        target_rarity = stringToCardRarity(rarity_filter_str);
    }

    for (const auto& pair : allCards_) {
        if (pair.second) { 
            if (any_rarity || pair.second->getRarity() == target_rarity) {
                eligibleCards.push_back(pair.second->cloneCard());
            }
        }
    }
    if (eligibleCards.empty()) {
        LOG_WARNING("game", "No eligible cards found for rarity_filter: " + rarity_filter_str + " in getRandomCardFromMasterList. Trying ANY rarity.");
        if (!any_rarity) {
            for (const auto& pair : allCards_) {
                if (pair.second) eligibleCards.push_back(pair.second->cloneCard());
            }
        }
        if (eligibleCards.empty()) return nullptr; 
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    return eligibleCards[gen() % eligibleCards.size()];
}

std::shared_ptr<Relic> Game::getRandomRelicFromMasterList() {
    std::vector<std::shared_ptr<Relic>> eligibleRelics;
    for (const auto& pair : allRelics_) {
        if (pair.second) { 
            eligibleRelics.push_back(pair.second->cloneRelic());
        }
    }
    if (eligibleRelics.empty()) {
        LOG_WARNING("game", "No relics found in getRandomRelicFromMasterList");
        return nullptr;
    }
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    return eligibleRelics[gen() % eligibleRelics.size()];
}

void Game::startShop() {
    LOG_INFO("game", "Starting shop and populating inventory...");
    shopCardsForSale_.clear();
    shopRelicsForSale_.clear();
    shopRelicPrices_.clear();

    // --- Populate Cards ---
    LOG_DEBUG("game_shop", "Total cards in allCards_: " + std::to_string(allCards_.size()));
    int numCardsToOffer = 3;
    std::vector<std::string> availableCardIds;
    for(const auto& pair : allCards_) {
        if (pair.second) {
            std::string rarityStr = "UNKNOWN_RARITY";
            
            if(pair.second->getRarity() == CardRarity::BASIC) rarityStr = "BASIC";
            else if(pair.second->getRarity() == CardRarity::COMMON) rarityStr = "COMMON";
            else if(pair.second->getRarity() == CardRarity::UNCOMMON) rarityStr = "UNCOMMON";
            else if(pair.second->getRarity() == CardRarity::RARE) rarityStr = "RARE";
            
            std::string typeStr = "UNKNOWN_TYPE";
            if(pair.second->getType() == CardType::ATTACK) typeStr = "ATTACK";
            else if(pair.second->getType() == CardType::SKILL) typeStr = "SKILL";
            else if(pair.second->getType() == CardType::POWER) typeStr = "POWER";
            else if(pair.second->getType() == CardType::STATUS) typeStr = "STATUS";
            else if(pair.second->getType() == CardType::CURSE) typeStr = "CURSE";

            bool passesFilter = (pair.second->getType() != CardType::CURSE && 
                                 pair.second->getType() != CardType::STATUS);
            LOG_DEBUG("game_shop", "Considering card: " + pair.second->getName() + 
                                   " | Rarity: " + rarityStr + 
                                   " | Type: " + typeStr + 
                                   " | PassesShopFilter: " + (passesFilter ? "Yes" : "No"));
            if (passesFilter) {
                 availableCardIds.push_back(pair.first);
            }
        } else {
            LOG_WARNING("game_shop", "Encountered null card in allCards_ with ID: " + pair.first);
        }
    }
    LOG_DEBUG("game_shop", "Number of cards available after filtering: " + std::to_string(availableCardIds.size()));
    std::shuffle(availableCardIds.begin(), availableCardIds.end(), rng_); 

    for (int i = 0; i < numCardsToOffer && i < static_cast<int>(availableCardIds.size()); ++i) {
        std::shared_ptr<Card> templateCard = allCards_[availableCardIds[i]];
        if (templateCard) {
            Card* shopCardInstance = new Card(*templateCard); 

            int price = 50;

            if      (shopCardInstance->getRarity() == CardRarity::COMMON) price = 20 + (rng_() % 9);    // 20-29
            else if (shopCardInstance->getRarity() == CardRarity::BASIC) price = 25 + (rng_() % 11);    // 35-46
            else if (shopCardInstance->getRarity() == CardRarity::UNCOMMON) price = 45 + (rng_() % 21); // 65-86
            else if (shopCardInstance->getRarity() == CardRarity::RARE) price = 70 + (rng_() % 31);     // 100-131
            
            shopCardInstance->setCost(price);
            shopCardsForSale_.push_back(shopCardInstance);
            LOG_DEBUG("game_shop", "Added card to shop: " + shopCardInstance->getName() + " for " + std::to_string(shopCardInstance->getCost()) + "G");
        }
    }

    // --- Populate Relics ---
    int numRelicsToOffer = 1;
    std::vector<std::string> availableRelicIds;
    for(const auto& pair : allRelics_) {
        bool playerHasRelic = false;
        if (player_) {
            for (const auto& playerRelic : player_->getRelics()) {
                if (playerRelic->getId() == pair.first) {
                    playerHasRelic = true;
                    break;
                }
            }
        }
        if (!playerHasRelic && pair.second) {
            availableRelicIds.push_back(pair.first);
        }
    }
    std::shuffle(availableRelicIds.begin(), availableRelicIds.end(), rng_);

    for (int i = 0; i < numRelicsToOffer && i < static_cast<int>(availableRelicIds.size()); ++i) {
        std::shared_ptr<Relic> templateRelic = allRelics_[availableRelicIds[i]];
        if (templateRelic) {
            Relic* shopRelicInstance = new Relic(*templateRelic);
            
            int price = 150;
            if (shopRelicInstance->getRarity() == RelicRarity::RARE) price = 250;
            else if (shopRelicInstance->getRarity() == RelicRarity::UNCOMMON) price = 200;
            else if (shopRelicInstance->getRarity() == RelicRarity::BOSS) price = 300;
            else if (shopRelicInstance->getRarity() == RelicRarity::SHOP) price = 120;
            
            shopRelicPrices_[shopRelicInstance] = price;
            
            shopRelicsForSale_.push_back(shopRelicInstance);
            LOG_DEBUG("game_shop", "Added relic to shop: " + shopRelicInstance->getName() + 
                      " with price " + std::to_string(price) + "G");
        }
    }
    LOG_INFO("game", "Shop populated with " + std::to_string(shopCardsForSale_.size()) + " cards and " + std::to_string(shopRelicsForSale_.size()) + " relics.");
}

bool Game::loadAllCharacters() {
    allCharacters_.clear();
    failedLoads_ = 0;
    std::string data_prefix = get_data_path_prefix();
    std::string characters_dir_path_str = data_prefix + "data/characters";
    fs::path characters_dir_path(characters_dir_path_str);

    LOG_DEBUG("game", "Loading all characters from directory: " + characters_dir_path.string());

    if (!fs::exists(characters_dir_path) || !fs::is_directory(characters_dir_path)) {
        LOG_ERROR("game", "Characters directory not found or is not a directory: " + characters_dir_path.string());
        return false;
    }

    try {
        for (const auto& entry : fs::directory_iterator(characters_dir_path)) {
            if (entry.is_regular_file() && entry.path().extension() == ".json") {
                std::string char_id_from_filename = entry.path().stem().string();
                LOG_DEBUG("game", "Attempting to load character '" + char_id_from_filename + "' from: " + entry.path().string());
                
                std::ifstream file(entry.path());
                if (!file.is_open()) {
                    LOG_ERROR("game", "Could not open character file: " + entry.path().string());
                    failedLoads_++;
                    continue;
                }
                
                json charJson;
                try {
                    file >> charJson;
                } catch (const json::parse_error& e) {
                    LOG_ERROR("game", "JSON parse error in file " + entry.path().string() + ": " + e.what());
                    failedLoads_++;
                    file.close();
                    continue;
                }
                file.close();

                CharacterData data;
                try {
                    data.id = charJson.value("id", char_id_from_filename);
                    data.name = charJson.at("name").get<std::string>();
                    data.max_health = charJson.at("max_health").get<int>();
                    data.base_energy = charJson.at("base_energy").get<int>();
                    data.initial_hand_size = charJson.at("initial_hand_size").get<int>();
                    data.description = charJson.value("description", "");

                    if (charJson.contains("starting_deck") && charJson["starting_deck"].is_array()) {
                        for (const auto& deck_item : charJson["starting_deck"]) {
                            data.starting_deck.push_back(deck_item.get<std::string>());
                        }
                    }
                    if (charJson.contains("starting_relics") && charJson["starting_relics"].is_array()) {
                        for (const auto& relic_item : charJson["starting_relics"]) {
                            data.starting_relics.push_back(relic_item.get<std::string>());
                        }
                    }
                    allCharacters_[data.id] = data;
                    LOG_INFO("game", "Successfully loaded character: " + data.name + " (ID: " + data.id + ")");
                } catch (const json::exception& e) {
                    LOG_ERROR("game", "Error processing JSON data for character file " + entry.path().string() + ": " + e.what());
                    failedLoads_++;
                }
            }
        }
        LOG_INFO("game", "Loaded " + std::to_string(allCharacters_.size()) + " characters. " + std::to_string(failedLoads_) + " failed to load.");
        return failedLoads_ == 0;
    } catch (const fs::filesystem_error& e) {
        LOG_ERROR("game", "Filesystem error while loading characters: " + std::string(e.what()));
        return false;
    }
}

} // namespace deckstiny 