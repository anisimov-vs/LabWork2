// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include "ui/text_ui.h"
#include "core/game.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/combat.h"
#include "core/card.h"
#include "core/relic.h"
#include "core/map.h"
#include "core/event.h"
#include "util/logger.h"

#include <iostream>
#include <iomanip>
#include <sstream>
#include <thread>
#include <chrono>
#include <algorithm>
#include <cctype>
#include <limits>
#include <map>
#include <set>
#include <atomic>

namespace deckstiny {

std::atomic<bool> TextUI::testingModeEnabled_(false);

void TextUI::setTestingMode(bool enabled) {
    testingModeEnabled_ = enabled;
}

bool TextUI::isTestingMode() {
    LOG_DEBUG("textui_testing_check", "isTestingMode() -> " + std::string(testingModeEnabled_ ? "true" : "false"));
    return testingModeEnabled_;
}

TextUI::TextUI() : running_(false) {
}

TextUI::~TextUI() {
    shutdown();
}

bool TextUI::initialize(Game* game) {
    if (isTestingMode()) {
        LOG_DEBUG("textui", "TextUI::initialize called in TESTING MODE - input thread NOT started.");
        game_ = game;
        running_ = true;
        return true;
    }
    LOG_DEBUG("textui", "TextUI::initialize called");
    game_ = game;
    running_ = true;
    
    inputThread_ = std::thread(&TextUI::inputThreadFunc, this);
    
    LOG_DEBUG("textui", "TextUI::initialize completed");
    return true;
}

void TextUI::run() {
    LOG_DEBUG("textui", "TextUI::run started");
}

void TextUI::shutdown() {
    if (running_) {
        running_ = false;
        
        inputCondition_.notify_all();
        
        if (inputThread_.joinable()) {
            inputThread_.join();
        }
    }
}

void TextUI::setInputCallback(std::function<bool(const std::string&)> callback) {
    inputCallback_ = callback;
}

void TextUI::showMainMenu() {
    LOG_DEBUG("textui_testing_check", "Enter TextUI::showMainMenu, isTestingMode() -> " + std::string(isTestingMode() ? "true" : "false"));
    if (isTestingMode()) return;
    clearScreen();
    drawLine();
    std::cout << centerString("DECKSTINY", 80) << std::endl;
    drawLine();
    std::cout << std::endl;
    std::cout << centerString("1. New Game", 80) << std::endl;
    std::cout << centerString("2. Quit", 80) << std::endl;
    std::cout << std::endl;
    drawLine();
    std::cout << "Enter your choice: ";
}

void TextUI::showCharacterSelection(const std::vector<std::string>& availableClasses) {
    if (isTestingMode()) return;
    LOG_DEBUG("textui", "Showing character selection screen");
    
    clearScreen();
    drawLine();
    std::cout << centerString("CHARACTER SELECTION", 80) << std::endl;
    drawLine();
    std::cout << std::endl;
    
    for (size_t i = 0; i < availableClasses.size(); ++i) {
        std::cout << centerString(std::to_string(i + 1) + ". " + availableClasses[i], 80) << std::endl;
    }
    
    std::cout << std::endl;
    drawLine();
    std::cout << "Choose your character (1-4): ";
    
    lastMessage = "CHARACTER SELECTION";
}

void TextUI::showMap(int currentRoomId, 
                     const std::vector<int>& availableRooms,
                     const std::unordered_map<int, Room>& allRooms) {
    if (isTestingMode()) return;
    clearScreen();
    drawLine();
    std::cout << centerString("MAP", 80) << std::endl;
    drawLine();
    std::cout << std::endl;
    
    auto currentIt = allRooms.find(currentRoomId);
    if (currentIt != allRooms.end()) {
        std::cout << "Current room: " << getRoomTypeString(currentIt->second.type) << std::endl;
        std::cout << std::endl;
    }
    
    std::cout << "Available rooms:" << std::endl;
    for (size_t i = 0; i < availableRooms.size(); i++) {
        auto it = allRooms.find(availableRooms[i]);
        if (it != allRooms.end()) {
            std::cout << (i + 1) << ". " << getRoomTypeString(it->second.type) << std::endl;
        }
    }
    
    std::cout << std::endl;
    drawLine();
    std::cout << "Enter room number to move to, or 'back' to return to main menu: ";
}

void TextUI::showCombat(const Combat* combat) {
    if (isTestingMode()) return;
    if (!combat) {
        return;
    }
    
    clearScreen();
    drawLine();
    std::cout << centerString("COMBAT - TURN " + std::to_string(combat->getTurn()), 80) << std::endl;
    drawLine();
    std::cout << std::endl;
    
    std::cout << "Enemies:" << std::endl;
    for (size_t i = 0; i < combat->getEnemyCount(); ++i) {
        Enemy* enemy = combat->getEnemy(i);
        if (enemy && enemy->isAlive()) {
            showEnemyStats(enemy);
        }
    }
    
    std::cout << std::endl;
    
    Player* player = combat->getPlayer();
    if (player) {
        showPlayerStats(player);
        
        if (combat->isPlayerTurn()) {
            std::cout << std::endl;
            std::cout << "Hand:" << std::endl;
            
            const auto& hand = player->getHand();
            for (size_t i = 0; i < hand.size(); ++i) {
                std::cout << i + 1 << ". ";
                showCard(hand[i].get(), true, false);
            }
        }
    }
    
    std::cout << std::endl;
    drawLine();
    
    if (combat->isPlayerTurn()) {
        std::cout << "Available Actions: " << std::endl;
        std::cout << "  Type a card number (1-" << player->getHand().size() << ") to play that card" << std::endl;
        std::cout << "  Type 'end' to end your turn" << std::endl;
        std::cout << "  Type 'help' for more information" << std::endl;
    } else {
        std::cout << "Enemies are taking their turns..." << std::endl;
    }
}

void TextUI::showPlayerStats(const Player* player) {
    if (isTestingMode()) return;
    if (!player) {
        return;
    }
    
    std::cout << "Player: " << player->getName() << std::endl;
    std::cout << "HP: " << player->getHealth() << "/" << player->getMaxHealth();
    
    if (player->getBlock() > 0) {
        std::cout << " (Block: " << player->getBlock() << ")";
    }
    
    std::cout << " | Energy: " << player->getEnergy() << "/" << player->getBaseEnergy() << std::endl;
    
    const auto& effects = player->getStatusEffects();
    if (!effects.empty()) {
        std::cout << "Status Effects: ";
        bool first = true;
        for (const auto& effect : effects) {
            if (!first) {
                std::cout << ", ";
            }
            std::cout << effect.first << " (" << effect.second << ")";
            first = false;
        }
        std::cout << std::endl;
    }
}

void TextUI::showEnemyStats(const Enemy* enemy) {
    if (isTestingMode()) return;
    if (!enemy) {
        return;
    }
    
    std::cout << "Enemy: " << enemy->getName() << std::endl;
    std::cout << "HP: " << enemy->getHealth() << "/" << enemy->getMaxHealth();
    
    if (enemy->getBlock() > 0) {
        std::cout << " \033[36m[BLOCK: " << enemy->getBlock() << "]\033[0m"; // Blue text for Block
    }
    
    std::cout << std::endl;
    
    const Intent& intent = enemy->getIntent();
    std::cout << "Intent: ";
    
    // Make intent display more descriptive with ASCII symbols
    if (intent.type == "attack") {
        std::cout << "\033[31m[ATTACK]\033[0m for " << intent.value << " damage"; // Red for attack
    } else if (intent.type == "attack_defend") {
        std::cout << "\033[31m[ATTACK]\033[0m for " << intent.value << " damage and \033[36m[BLOCK]\033[0m (" << intent.secondaryValue << ")";
    } else if (intent.type == "defend") {
        std::cout << "\033[36m[DEFEND]\033[0m (gain " << intent.value << " Block)";
    } else if (intent.type == "buff") {
        std::cout << "\033[32m[BUFF]\033[0m"; // Green for buff
        if (!intent.effect.empty()) {
            std::cout << " (" << intent.effect << " +" << intent.value << ")";
        }
        
        // Special case for buff intents that also add block
        if (intent.type == "buff" && intent.effect == "strength") {
            std::cout << " and \033[36m[BLOCK]\033[0m (6)";
        }
    } else if (intent.type == "debuff") {
        std::cout << "\033[35m[DEBUFF]\033[0m"; // Purple for debuff
        if (!intent.effect.empty()) {
            std::cout << " (" << intent.effect << " +" << intent.value << ")";
        }
    } else {
        // Fallback for other intent types
        std::cout << "[" << intent.type << "]";
        if (intent.value > 0) {
            std::cout << " (" << intent.value << ")";
        }
    }
    std::cout << std::endl;
    
    const auto& effects = enemy->getStatusEffects();
    if (!effects.empty()) {
        std::cout << "Status Effects: ";
        bool first = true;
        for (const auto& effect : effects) {
            if (!first) {
                std::cout << ", ";
            }
            
            if (effect.first == "strength") {
                std::cout << "\033[32mStrength\033[0m"; // Green for positive effects
            } else if (effect.first == "vulnerable" || effect.first == "weak") {
                std::cout << "\033[35m" << effect.first << "\033[0m"; // Purple for negative effects
            } else {
                std::cout << effect.first;
            }
            
            std::cout << " (" << effect.second << ")";
            first = false;
        }
        std::cout << std::endl;
    }
    
    std::cout << std::endl;
}

void TextUI::showCard(const Card* card, bool showEnergyCost, bool selected) {
    if (isTestingMode()) return;
    if (!card) {
        return;
    }
    
    std::stringstream ss;
    
    if (showEnergyCost) {
        ss << "[" << card->getCost() << "] ";
    }
    
    ss << card->getName() << " (" << getCardTypeString(card->getType()) << ")";
    
    if (card->isUpgraded()) {
        ss << " +";
    }
    
    if (selected) {
        std::cout << "> " << ss.str() << " <" << std::endl;
    } else {
        std::cout << ss.str() << std::endl;
    }
    
    std::cout << "    " << card->getDescription() << std::endl;
}

void TextUI::showCards(const std::vector<Card*>& cards, 
                       const std::string& title, 
                       bool showIndices) {
    if (isTestingMode()) return;
    if (!title.empty()) {
        std::cout << title << ":" << std::endl;
    }
    
    if (cards.empty()) {
        std::cout << "    No cards." << std::endl;
        return;
    }
    
    for (size_t i = 0; i < cards.size(); ++i) {
        if (showIndices) {
            std::cout << i + 1 << ". ";
        } else {
            std::cout << "- ";
        }
        
        showCard(cards[i], true, false);
    }
}

void TextUI::showRelic(const Relic* relic) {
    if (isTestingMode()) return;
    if (!relic) {
        return;
    }
    
    std::cout << relic->getName() << " (" << getRelicRarityString(relic->getRarity()) << ")" << std::endl;
    std::cout << "    " << relic->getDescription() << std::endl;
    
    if (!relic->getFlavorText().empty()) {
        std::cout << "    \"" << relic->getFlavorText() << "\"" << std::endl;
    }
}

void TextUI::showRelics(const std::vector<Relic*>& relics, const std::string& title) {
    if (isTestingMode()) return;
    if (!title.empty()) {
        std::cout << title << ":" << std::endl;
    }
    
    if (relics.empty()) {
        std::cout << "    No relics." << std::endl;
        return;
    }
    
    for (size_t i = 0; i < relics.size(); ++i) {
        std::cout << i + 1 << ". ";
        showRelic(relics[i]);
    }
}

void TextUI::showMessage(const std::string& message, bool error) {
    lastMessage = message;
    
    if (error) {
        if (isTestingMode()) {
            LOG_ERROR("textui_showMessage", "Error message (suppressed in test): " + message);
            return;
        }
        std::cout << std::endl << "--------------------------------------------------------------------------------" << std::endl;
        std::cout << message << std::endl;
        std::cout << "--------------------------------------------------------------------------------" << std::endl;
        
        std::cout << std::endl << "Press Enter to continue...";
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        // Clear screen after user presses Enter
        std::cout << "\033[2J\033[1;1H";
    } else {
        if (isTestingMode()) {
            LOG_INFO("textui_showMessage", "Message (suppressed in test): " + message);
            return;
        }
        std::cout << std::endl << message << std::endl << std::endl;
    }
}

std::string TextUI::getInput(const std::string& prompt) {
    // Store the last prompt
    lastInputPrompt = prompt;
    
    if (isTestingMode()) {
        LOG_DEBUG("textui_getInput", "getInput called in TESTING MODE with prompt: \"" + prompt + "\". Returning empty string.");
        return ""; 
    }
    
    if (!prompt.empty()) {
        std::cout << prompt;
    }
    
    std::string input;
    std::getline(std::cin, input);
    
    if (input == "help" || input == "h") {
        if (lastMessage.find("EVENT") != std::string::npos) {
            showEventHelp();
            return getInput(prompt);
        } else if (lastMessage.find("COMBAT") != std::string::npos) {
            showCombatHelp();
            return getInput(prompt);
        } else if (lastMessage.find("MAP") != std::string::npos) {
            showMapHelp();
            return getInput(prompt);
        } else {
            std::cout << "No specific help available for current context." << std::endl;
            return getInput(prompt);
        }
    }
    
    return input;
}

void TextUI::clearScreen() const {
    if (isTestingMode()) return;
    std::cout << "\033[H\033[2J\033[3J"; // ANSI escape codes for clearing screen
    std::cout.flush();
}

void TextUI::update() {
    if (isTestingMode()) return;
}

void TextUI::showRewards(int gold, 
                         const std::vector<Card*>& cards,
                         const std::vector<Relic*>& relics) {
    if (isTestingMode()) return;
    Player* player = nullptr;
    if (game_) {
        player = game_->getPlayer();
    }
    
    clearScreen();
    drawLine();
    std::cout << centerString("COMBAT REWARDS", 80) << std::endl;
    drawLine();
    std::cout << std::endl;
    
    std::cout << "Gold earned: " << gold << std::endl;
    
    if (player) {
        std::cout << "Total gold: " << player->getGold() << std::endl;
    }
    std::cout << std::endl;
    
    if (!cards.empty()) {
        std::cout << "Card Rewards:" << std::endl;
        showCards(cards, "", true);
        std::cout << std::endl;
    }
    
    if (!relics.empty()) {
        std::cout << "Relic Rewards:" << std::endl;
        showRelics(relics, "");
        std::cout << std::endl;
    }
    
    drawLine();
    waitForKeyPress();
}

void TextUI::showGameOver(bool victory, int score) {
    if (isTestingMode()) {
        LOG_DEBUG("textui_showGameOver", "showGameOver called in TESTING MODE. Suppressing UI and input.");
        return;
    }
    clearScreen();
    drawLine('=');
    std::cout << std::endl;
    
    if (victory) {
        std::cout << centerString("*** VICTORY! ***", 80) << std::endl;
        std::cout << centerString("Congratulations!", 80) << std::endl;
    } else {
        std::cout << centerString("*** GAME OVER ***", 80) << std::endl;
        std::cout << centerString("You have been defeated!", 80) << std::endl;
    }
    
    std::cout << std::endl;
    drawLine('-');
    std::cout << std::endl;
    
    std::cout << centerString("Final Score: " + std::to_string(score), 80) << std::endl;
    std::cout << std::endl;
    
    lastMessage = "GAME_OVER";
    
    drawLine('=');
    std::cout << std::endl;
    std::cout << centerString("Press Enter to return to main menu...", 80) << std::endl;
    
    if (inputCallback_ && game_) {
        LOG_INFO("textui", "Game over screen: Forcing immediate transition to main menu");
        
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        std::cin.clear();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        
        inputCallback_("continue");
    }
}

void TextUI::inputThreadFunc() {
    if (isTestingMode()) return;

    LOG_DEBUG("textui", "Input thread started");
    
    while (running_) {
        std::string input;
        
        if (std::cin.good()) {
            std::getline(std::cin, input);
            
            LOG_DEBUG("textui", "Input thread received: '" + input + "'");
            
            if (inputCallback_ && (!input.empty() || std::cin.eof())) {
                if (input == "quit" || input == "exit") {
                    LOG_INFO("textui", "User requested exit");
                    if (game_) {
                        game_->shutdown();
                    }
                    break;
                }
                
                inputCallback_(input);
            }
        } else {
            LOG_WARNING("textui", "Input stream in bad state, clearing flags");
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
    
    LOG_DEBUG("textui", "Input thread stopped");
}

std::string TextUI::getRoomTypeString(RoomType type) const {
    switch (type) {
        case RoomType::MONSTER:  return "Monster";
        case RoomType::ELITE:    return "Elite";
        case RoomType::BOSS:     return "Boss";
        case RoomType::REST:     return "Rest Site";
        case RoomType::EVENT:    return "Event";
        case RoomType::SHOP:     return "Shop";
        case RoomType::TREASURE: return "Treasure";
        default:                 return "Unknown Room";
    }
}

std::string TextUI::getCardTypeString(CardType type) const {
    switch (type) {
        case CardType::ATTACK:  return "Attack";
        case CardType::SKILL:   return "Skill";
        case CardType::POWER:   return "Power";
        case CardType::STATUS:  return "Status";
        case CardType::CURSE:   return "Curse";
        default:                return "Unknown";
    }
}

std::string TextUI::getCardRarityString(CardRarity rarity) const {
    switch (rarity) {
        case CardRarity::COMMON:    return "Common";
        case CardRarity::UNCOMMON:  return "Uncommon";
        case CardRarity::RARE:      return "Rare";
        case CardRarity::SPECIAL:   return "Special";
        case CardRarity::BASIC:     return "Basic";
        default:                    return "Unknown";
    }
}

std::string TextUI::getRelicRarityString(RelicRarity rarity) const {
    switch (rarity) {
        case RelicRarity::STARTER:   return "Starter";
        case RelicRarity::COMMON:    return "Common";
        case RelicRarity::UNCOMMON:  return "Uncommon";
        case RelicRarity::RARE:      return "Rare";
        case RelicRarity::BOSS:      return "Boss";
        case RelicRarity::SHOP:      return "Shop";
        case RelicRarity::EVENT:     return "Event";
        default:                     return "Unknown";
    }
}

void TextUI::drawLine(int width, std::ostream& os) const {
    if (isTestingMode()) return;
    os << std::string(width, '-') << std::endl;
}

std::string TextUI::centerString(const std::string& str, int width) const {
    if (isTestingMode()) return "";
    int padding = width - static_cast<int>(str.length());
    if (padding <= 0) {
        return str;
    }
    
    int leftPadding = padding / 2;
    return std::string(leftPadding, ' ') + str;
}

void TextUI::waitForKeyPress() {
    if (isTestingMode()) return;

    std::cout << std::endl << "Press Enter to continue...";
    std::cout.flush();
    
    std::string input;
    std::getline(std::cin, input);
    
    if (inputCallback_ && game_) {
        GameState currentState = game_->getCurrentState();
        LOG_INFO("textui", "waitForKeyPress calling input callback in state: " + 
            std::to_string(static_cast<int>(currentState)));
            
        inputCallback_("continue");
    } else if (!inputCallback_) {
        LOG_WARNING("textui", "waitForKeyPress: No input callback registered");
    } else if (!game_) {
        LOG_WARNING("textui", "waitForKeyPress: No game pointer available");
    }
    
    clearScreen();
}

void TextUI::showEnemySelectionMenu(const Combat* combat, const std::string& cardName) {
    if (isTestingMode()) return;
    if (!combat) {
        return;
    }
    
    clearScreen();
    drawLine();
    std::cout << centerString("SELECT TARGET FOR " + cardName, 80) << std::endl;
    drawLine();
    std::cout << std::endl;
    
    std::cout << "Enemies:" << std::endl;
    for (size_t i = 0; i < combat->getEnemyCount(); ++i) {
        Enemy* enemy = combat->getEnemy(i);
        if (enemy && enemy->isAlive()) {
            std::cout << i + 1 << ". ";
            showEnemyStats(enemy);
        }
    }
    
    std::cout << std::endl;
    drawLine();
    std::cout << "Enter enemy number to target, or 'cancel' to select a different card: ";
}

void TextUI::showEvent(const Event* event, const Player* player) {
    if (isTestingMode()) return;
    if (!event || !player) {
        LOG_ERROR("textui", "Invalid event or player");
        return;
    }
    
    std::cout << "\033[2J\033[1;1H";
    
    printDivider();
    std::cout << centerString("EVENT: " + event->getName(), 80) << std::endl;
    printDivider();
    
    std::cout << event->getDescription() << std::endl << std::endl;
    
    std::cout << "Player Status: ";
    std::cout << "HP: " << player->getHealth() << "/" << player->getMaxHealth();
    std::cout << " | Gold: " << player->getGold();
    std::cout << std::endl << std::endl;
    
    const std::vector<EventChoice>& choices = event->getAvailableChoices(const_cast<Player*>(player));
    
    if (choices.empty()) {
        std::cout << "This event has no available choices. Type 'back' to return to the map." << std::endl;
    } else {
        std::cout << "Available Choices:" << std::endl;
        for (size_t i = 0; i < choices.size(); ++i) {
            const EventChoice& choice = choices[i];
            std::cout << (i + 1) << ". " << choice.text;
            
            if (choice.goldCost > 0) {
                std::cout << " (Requires " << choice.goldCost << " Gold)";
            }
            if (choice.healthCost > 0) {
                std::cout << " (Requires " << choice.healthCost << " HP)";
            }
            
            std::cout << std::endl;
        }
        
        std::cout << std::endl;
        std::cout << "Enter choice number or 'back' to return to the map: ";
    }
}

void TextUI::showEventResult(const std::string& resultText) {
    if (isTestingMode()) return;
    LOG_DEBUG("textui", "Showing event result: \" + resultText + \"");
    clearScreen();
    drawLine();
    std::cout << centerString("EVENT RESULT", 80) << std::endl;
    drawLine();
    std::cout << std::endl;
    std::cout << resultText << std::endl;
    std::cout << std::endl;
    drawLine();
    std::cout << "Press Enter to continue...";
    if (!isTestingMode()) {
      std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
      clearScreen();
    }
}

void TextUI::showEventHelp() {
    std::cout << "Event Commands:" << std::endl;
    std::cout << "  [number] - Select a choice by its number" << std::endl;
    std::cout << "  back - Return to the map" << std::endl;
    std::cout << "  help - Show this help message" << std::endl;
}

void TextUI::showMapHelp() {
    std::cout << "Map Commands:" << std::endl;
    std::cout << "  [number] - Move to a room by its number" << std::endl;
    std::cout << "  back - Return to the main menu" << std::endl;
    std::cout << "  help - Show this help message" << std::endl;
}

void TextUI::showCombatHelp() {
    std::cout << "Combat Commands:" << std::endl;
    std::cout << "  [number] - Play a card by its number" << std::endl;
    std::cout << "  end - End your turn" << std::endl;
    std::cout << "  back - Return to map (only if combat is not started)" << std::endl;
    std::cout << "  help - Show this help message" << std::endl;
    std::cout << "  inspect [e/enemy] [number] - Inspect an enemy by number" << std::endl;
    std::cout << "  inspect [c/card] [number] - Inspect a card by number" << std::endl;
    std::cout << "  inspect [p/player] - Inspect your character" << std::endl;
    std::cout << std::endl;
    std::cout << "Card Targeting:" << std::endl;
    std::cout << "  1. For single-enemy cards: " << std::endl;
    std::cout << "     - If only one enemy is present, it's targeted automatically" << std::endl;
    std::cout << "     - With multiple enemies, select a card then choose a target" << std::endl;
    std::cout << "  2. For multi-enemy/non-targeting cards: played immediately" << std::endl;
}

void TextUI::printDivider(int width) {
    if (isTestingMode()) return;
    std::cout << std::string(width, '=') << std::endl;
}

void TextUI::showShop(const std::vector<Card*>& cards, 
                        const std::vector<Relic*>& relics, 
                        int playerGold) {
    // Call the overloaded version with an empty relic prices map
    std::unordered_map<Relic*, int> emptyPrices;
    showShop(cards, relics, emptyPrices, playerGold);
}

void TextUI::showShop(const std::vector<Card*>& cards, 
                        const std::vector<Relic*>& relics,
                        const std::unordered_map<Relic*, int>& relicPrices,
                        int playerGold) {
    LOG_DEBUG("textui_shop_check", "TextUI::showShop called. Received cards.size() = " + std::to_string(cards.size()) + ", relics.size() = " + std::to_string(relics.size()));
    if (isTestingMode()) return;
    clearScreen();
    drawLine();
    std::cout << centerString("MERCHANT", 80) << std::endl;
    drawLine();
    std::cout << std::endl;
    std::cout << "Your Gold: " << playerGold << std::endl << std::endl;

    std::cout << "--- Cards for Sale ---" << std::endl;
    if (cards.empty()) {
        std::cout << "No cards available for sale right now." << std::endl;
    } else {
        for (size_t i = 0; i < cards.size(); ++i) {
            const Card* card = cards[i];
            if (card) {
                int price = card->getCost();

                std::cout << "C" << (i + 1) << ". " << card->getName() 
                          << " (Cost: " << price << "G) - " 
                          << card->getDescription() << std::endl;
            }
        }
    }
    std::cout << std::endl;

    std::cout << "--- Relics for Sale ---" << std::endl;
    if (relics.empty()) {
        std::cout << "No relics available for sale right now." << std::endl;
    } else {
        for (size_t i = 0; i < relics.size(); ++i) {
            const Relic* relic = relics[i];
            if (relic) {
                // Get price from the map if available
                int price = 150; // Default price for common relics
                auto priceIt = relicPrices.find(const_cast<Relic*>(relic));
                if (priceIt != relicPrices.end()) {
                    price = priceIt->second;
                } else {
                    // Calculate based on rarity if not found in map
                    if (relic->getRarity() == RelicRarity::RARE) price = 250;
                    else if (relic->getRarity() == RelicRarity::UNCOMMON) price = 200;
                    else if (relic->getRarity() == RelicRarity::BOSS) price = 300;
                    else if (relic->getRarity() == RelicRarity::SHOP) price = 120;
                }

                std::cout << "R" << (i + 1) << ". " << relic->getName()
                          << " (Cost: " << price << "G) - "
                          << relic->getDescription() << std::endl;
            }
        }
    }
    std::cout << std::endl;

    drawLine();
    std::cout << "Enter item to buy (e.g., C1, R2) or 'leave': ";
}
} // namespace deckstiny 
