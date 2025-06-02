// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#pragma once

#include "ui/ui_interface.h"
#include "core/game.h"
#include "core/map.h"
#include <string>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <functional>
#include <memory>

namespace deckstiny {

namespace testing {

class MockUI : public UIInterface {
public:
    MockUI();
    ~MockUI() override;

    bool initialize(Game* game) override;
    void run() override;
    void shutdown() override;
    void setInputCallback(std::function<bool(const std::string&)> callback) override;
    void showMainMenu() override;
    void showCharacterSelection(const std::vector<std::string>& availableClasses) override;
    void showMap(int currentRoomId,
                 const std::vector<int>& availableRooms,
                 const std::unordered_map<int, Room>& allRooms) override;
    void showCombat(const Combat* combat) override;
    void showPlayerStats(const Player* player) override;
    void showEnemyStats(const Enemy* enemy) override;
    void showEnemySelectionMenu(const Combat* combat, const std::string& cardName) override;
    void showCard(const Card* card, bool showEnergyCost = true, bool selected = false) override;
    void showCards(const std::vector<Card*>& cards,
                   const std::string& title = "",
                   bool showIndices = true) override;
    void showRelic(const Relic* relic) override;
    void showRelics(const std::vector<Relic*>& relics,
                    const std::string& title = "") override;
    void showMessage(const std::string& message, bool pause = false) override;
    std::string getInput(const std::string& prompt) override;
    void clearScreen() const override;
    void update() override;
    void showRewards(int gold,
                     const std::vector<Card*>& cards,
                     const std::vector<Relic*>& relics) override;
    void showGameOver(bool victory, int score) override;
    void showEvent(const Event* event, const Player* player) override;
    void showEventResult(const std::string& resultText) override;
    void showShop(const std::vector<Card*>& cardsForSale,
                  const std::vector<Relic*>& relicsForSale,
                  int playerGold) override;

    // Mock-specific methods
    void addExpectedInput(const std::string& input);
    bool triggerNextInput();
    bool wasMethodCalled(const std::string& methodName) const;
    int getMethodCallCount(const std::string& methodName) const;
    void clearRecordedCalls();
    void waitForInput();

    // Getters for last shown data
    std::string getLastMessageText() const { return lastMessageText_; }
    bool getLastMessagePauseState() const { return lastMessagePause_; }
    const std::vector<std::string>& getDisplayedMessagesHistory() const { return displayedMessagesHistory_; } // Renamed for clarity
    GameState getLastShownState() const { return lastShownState_; }
    int getLastGoldReward() const { return lastGoldReward_; }
    const std::vector<Card*>& getLastCardRewards() const { return lastCardRewards_; }
    const std::vector<Relic*>& getLastRelicRewards() const { return lastRelicRewards_; }

    // Public members to store last passed data for easy access in tests
    int lastCurrentRoomId_ = -1;
    std::vector<int> lastAvailableRooms_;
    std::unordered_map<int, Room> lastAllRooms_;
    const Combat* lastCombat_ = nullptr;
    std::vector<std::string> lastAvailableClasses_;
    const Player* lastPlayerStats_ = nullptr;
    const Enemy* lastEnemyStats_ = nullptr;
    const Combat* lastCombatForEnemySelection_ = nullptr;
    std::string   lastCardNameForEnemySelection_;
    const Card* lastShownCard_ = nullptr;
    bool lastShowEnergyCost_ = true;
    bool lastCardSelected_ = false;
    std::vector<Card*> lastShownCards_;
    std::string lastCardsTitle_;
    bool lastShowIndices_ = true;
    const Relic* lastShownRelic_ = nullptr;
    std::vector<Relic*> lastShownRelics_;
    std::string lastRelicsTitle_;
    bool lastVictoryState_ = false;
    int lastScore_ = 0;
    const Event* lastShownEvent_ = nullptr;
    const Player* lastPlayerForEvent_ = nullptr;
    std::string lastEventResultText_;
    std::string lastInputPrompt_;

    // Data for showShop
    std::vector<Card*> lastCardsForSale_;
    std::vector<Relic*> lastRelicsForSale_;
    int lastPlayerGoldForShop_ = 0;

private:
    Game* game_ = nullptr;
    std::function<bool(const std::string&)> inputCallback_;
    std::deque<std::string> expectedInputs_;
    std::vector<std::string> displayedMessagesHistory_; // Stores all messages shown via showMessage
    mutable std::map<std::string, int> methodCallCounts_;
    bool originalConsoleState_ = true;

    GameState lastShownState_ = GameState::MAIN_MENU;
    std::string lastMessageText_;    // For the last call to showMessage
    bool lastMessagePause_ = false;  // For the last call to showMessage
    int lastGoldReward_ = 0;
    std::vector<Card*> lastCardRewards_;
    std::vector<Relic*> lastRelicRewards_;

    void recordMethodCall(const std::string& methodName) const;
};

} // namespace testing
} // namespace deckstiny 