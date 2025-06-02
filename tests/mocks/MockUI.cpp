#include "mocks/MockUI.h"
#include "core/game.h"
#include "util/logger.h"
#include <iostream>

namespace deckstiny {
namespace testing {

MockUI::MockUI() {
    if (util::Logger::isInitialized()) {
        originalConsoleState_ = util::Logger::getInstance().isConsoleEnabled();
        util::Logger::getInstance().setConsoleEnabled(false);
    }
    LOG_DEBUG("MockUI", "MockUI constructor");
    clearRecordedCalls();
}

MockUI::~MockUI() {
    LOG_DEBUG("MockUI", "MockUI destructor");
    if (util::Logger::isInitialized()) {
        util::Logger::getInstance().setConsoleEnabled(originalConsoleState_);
    }
}

bool MockUI::initialize(Game* game) {
    recordMethodCall("initialize");
    game_ = game;
    return true;
}

void MockUI::run() {
    recordMethodCall("run");
}

void MockUI::shutdown() {
    recordMethodCall("shutdown");
}

void MockUI::setInputCallback(std::function<bool(const std::string&)> callback) {
    recordMethodCall("setInputCallback");
    inputCallback_ = callback;
}

void MockUI::showMainMenu() {
    recordMethodCall("showMainMenu");
    lastShownState_ = GameState::MAIN_MENU;
}

void MockUI::showCharacterSelection(const std::vector<std::string>& availableClasses) {
    recordMethodCall("showCharacterSelection");
    lastShownState_ = GameState::CHARACTER_SELECT;
    lastAvailableClasses_ = availableClasses;
}

void MockUI::showMap(int currentRoomId, const std::vector<int>& availableRooms, const std::unordered_map<int, Room>& allRooms) {
    recordMethodCall("showMap");
    lastShownState_ = GameState::MAP;
    lastCurrentRoomId_ = currentRoomId;
    lastAvailableRooms_ = availableRooms;
    lastAllRooms_ = allRooms;
}

void MockUI::showCombat(const Combat* combat) {
    recordMethodCall("showCombat");
    lastShownState_ = GameState::COMBAT;
    lastCombat_ = combat;
}

void MockUI::showPlayerStats(const Player* player) {
    recordMethodCall("showPlayerStats");
    lastPlayerStats_ = player;
}

void MockUI::showEnemyStats(const Enemy* enemy) {
    recordMethodCall("showEnemyStats");
    lastEnemyStats_ = enemy;
}

void MockUI::showEnemySelectionMenu(const Combat* combat, const std::string& cardName) {
    recordMethodCall("showEnemySelectionMenu");
    lastCombatForEnemySelection_ = combat;
    lastCardNameForEnemySelection_ = cardName;
}

void MockUI::showCard(const Card* card, bool showEnergyCost, bool selected) {
    recordMethodCall("showCard");
    lastShownCard_ = card;
    lastShowEnergyCost_ = showEnergyCost;
    lastCardSelected_ = selected;
}

void MockUI::showCards(const std::vector<Card*>& cards, const std::string& title, bool showIndices) {
    recordMethodCall("showCards");
    lastShownCards_ = cards;
    lastCardsTitle_ = title;
    lastShowIndices_ = showIndices;
}

void MockUI::showRelic(const Relic* relic) {
    recordMethodCall("showRelic");
    lastShownRelic_ = relic;
}

void MockUI::showRelics(const std::vector<Relic*>& relics, const std::string& title) {
    recordMethodCall("showRelics");
    lastShownRelics_ = relics;
    lastRelicsTitle_ = title;
}

void MockUI::showMessage(const std::string& message, bool pause) {
    recordMethodCall("showMessage");
    lastMessageText_ = message;
    lastMessagePause_ = pause;
    displayedMessagesHistory_.push_back(message); 
}

std::string MockUI::getInput(const std::string& prompt) {
    recordMethodCall("getInput");
    lastInputPrompt_ = prompt;
    if (!expectedInputs_.empty()) {
        std::string input = expectedInputs_.front();
        expectedInputs_.pop_front();
        return input;
    }
    LOG_WARNING("MockUI", "getInput called but no expected input was set. Prompt: " + prompt);
    return "";
}

void MockUI::clearScreen() const {
    recordMethodCall("clearScreen");
}

void MockUI::update() {
    recordMethodCall("update");
}

void MockUI::showRewards(int gold, const std::vector<Card*>& cards, const std::vector<Relic*>& relics) {
    recordMethodCall("showRewards");
    lastShownState_ = GameState::REWARD;
    lastGoldReward_ = gold;
    lastCardRewards_ = cards;
    lastRelicRewards_ = relics;
}

void MockUI::showGameOver(bool victory, int score) {
    recordMethodCall("showGameOver");
    lastShownState_ = GameState::GAME_OVER;
    lastVictoryState_ = victory;
    lastScore_ = score;
}

void MockUI::showEvent(const Event* event, const Player* player) {
    recordMethodCall("showEvent");
    lastShownState_ = GameState::EVENT;
    lastShownEvent_ = event;
    lastPlayerForEvent_ = player;
}

void MockUI::showEventResult(const std::string& resultText) {
    recordMethodCall("showEventResult");
    lastEventResultText_ = resultText;
}

void MockUI::showShop(const std::vector<Card*>& cardsForSale,
                        const std::vector<Relic*>& relicsForSale,
                        int playerGold) {
    recordMethodCall("showShop");
    lastShownState_ = GameState::SHOP;
    lastCardsForSale_ = cardsForSale;
    lastRelicsForSale_ = relicsForSale;
    lastPlayerGoldForShop_ = playerGold;
}

void MockUI::addExpectedInput(const std::string& input) {
    recordMethodCall("addExpectedInput");
    expectedInputs_.push_back(input);
}

bool MockUI::triggerNextInput() {
    recordMethodCall("triggerNextInput");
    if (!expectedInputs_.empty() && inputCallback_) {
        std::string input = expectedInputs_.front();
        expectedInputs_.pop_front();
        LOG_DEBUG("MockUI", "Triggering input: " + input);
        return inputCallback_(input);
    }
    LOG_WARNING("MockUI", "No input to trigger or callback not set.");
    return false;
}

bool MockUI::wasMethodCalled(const std::string& methodName) const {
    return methodCallCounts_.count(methodName) > 0;
}

int MockUI::getMethodCallCount(const std::string& methodName) const {
    auto it = methodCallCounts_.find(methodName);
    if (it != methodCallCounts_.end()) {
        return it->second;
    }
    return 0;
}

void MockUI::clearRecordedCalls() {
    methodCallCounts_.clear();
    expectedInputs_.clear();
    displayedMessagesHistory_.clear();
    lastMessageText_.clear();
    lastMessagePause_ = false;
    lastShownState_ = GameState::MAIN_MENU; 
    lastGoldReward_ = 0;
    lastCardRewards_.clear();
    lastRelicRewards_.clear();
    lastCurrentRoomId_ = -1;
    lastAvailableRooms_.clear();
    lastAllRooms_.clear();
    lastCombat_ = nullptr;
    lastAvailableClasses_.clear();
    lastPlayerStats_ = nullptr;
    lastEnemyStats_ = nullptr;
    lastCombatForEnemySelection_ = nullptr;
    lastCardNameForEnemySelection_.clear();
    lastShownCard_ = nullptr;
    lastShowEnergyCost_ = true;
    lastCardSelected_ = false;
    lastShownCards_.clear();
    lastCardsTitle_.clear();
    lastShowIndices_ = true;
    lastShownRelic_ = nullptr;
    lastShownRelics_.clear();
    lastRelicsTitle_.clear();
    lastVictoryState_ = false;
    lastScore_ = 0;
    lastShownEvent_ = nullptr;
    lastPlayerForEvent_ = nullptr;
    lastEventResultText_.clear();
    lastInputPrompt_.clear();
    game_ = nullptr;
    inputCallback_ = nullptr;
}

void MockUI::waitForInput() {
    recordMethodCall("waitForInput");
}

void MockUI::recordMethodCall(const std::string& methodName) const {
    methodCallCounts_[methodName]++;
    LOG_DEBUG("MockUI", "Method called: " + methodName + " (Count: " + std::to_string(methodCallCounts_[methodName]) + ")");
}

} // namespace testing
} // namespace deckstiny 