#pragma once

#include "ui/ui_interface.h"
#include "core/map.h"        // For RoomType
#include "core/card.h"       // For CardType, CardRarity
#include "core/relic.h"      // For RelicRarity

#include <iostream>
#include <sstream>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <queue>

namespace deckstiny {

/**
 * @class TextUI
 * @brief Text-based implementation of UIInterface
 * 
 * Provides a console-based user interface for the game.
 */
class TextUI : public UIInterface {
public:
    /**
     * @brief Default constructor
     */
    TextUI();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~TextUI();
    
    /**
     * @brief Initialize the UI
     * @param game Pointer to the game instance
     * @return True if initialization succeeded, false otherwise
     */
    bool initialize(Game* game) override;
    
    /**
     * @brief Run the UI main loop
     */
    void run() override;
    
    /**
     * @brief Shut down the UI
     */
    void shutdown() override;
    
    /**
     * @brief Set input callback
     * @param callback Function to call when input is received
     */
    void setInputCallback(std::function<bool(const std::string&)> callback) override;
    
    /**
     * @brief Display the main menu
     */
    void showMainMenu() override;
    
    /**
     * @brief Display the character selection screen
     * @param availableClasses List of available character classes
     */
    void showCharacterSelection(const std::vector<std::string>& availableClasses) override;
    
    /**
     * @brief Show the game map
     * @param currentRoomId ID of the current room
     * @param availableRooms List of available room IDs
     * @param allRooms Map of all rooms
     */
    void showMap(int currentRoomId, 
                 const std::vector<int>& availableRooms,
                 const std::unordered_map<int, Room>& allRooms) override;
    
    /**
     * @brief Display combat state
     * @param combat Current combat instance
     */
    void showCombat(const Combat* combat) override;
    
    /**
     * @brief Display player stats
     * @param player Player character
     */
    void showPlayerStats(const Player* player) override;
    
    /**
     * @brief Display enemy stats
     * @param enemy Enemy character
     */
    void showEnemyStats(const Enemy* enemy) override;
    
    /**
     * @brief Display an enemy selection menu
     * @param combat Current combat instance
     * @param cardName Name of the card being played
     */
    void showEnemySelectionMenu(const Combat* combat, const std::string& cardName) override;
    
    /**
     * @brief Display a card
     * @param card Card to display
     * @param showEnergyCost Whether to show energy cost
     * @param selected Whether the card is selected
     */
    void showCard(const Card* card, bool showEnergyCost = true, bool selected = false) override;
    
    /**
     * @brief Display a list of cards
     * @param cards Cards to display
     * @param title Optional title for the card list
     * @param showIndices Whether to show indices for selection
     */
    void showCards(const std::vector<Card*>& cards, 
                   const std::string& title = "",
                   bool showIndices = true) override;
    
    /**
     * @brief Display a relic
     * @param relic Relic to display
     */
    void showRelic(const Relic* relic) override;
    
    /**
     * @brief Display a list of relics
     * @param relics Relics to display
     * @param title Optional title for the relic list
     */
    void showRelics(const std::vector<Relic*>& relics,
                    const std::string& title = "") override;
    
    /**
     * @brief Display a message
     * @param message Message to display
     * @param pause Whether to pause for user acknowledgement
     */
    void showMessage(const std::string& message, bool pause = false) override;
    
    /**
     * @brief Display a prompt and get input
     * @param prompt Prompt to display
     * @return User input string
     */
    std::string getInput(const std::string& prompt) override;
    
    /**
     * @brief Clear the screen
     */
    void clearScreen() const override;
    
    /**
     * @brief Wait for a key press
     */
    void waitForKeyPress();
    
    /**
     * @brief Draw a horizontal line
     * @param width Width of the line
     * @param os Output stream to write to
     */
    void drawLine(int width = 80, std::ostream& os = std::cout) const;
    
    /**
     * @brief Update the display
     */
    void update() override;
    
    /**
     * @brief Display rewards after combat
     * @param gold Gold reward
     * @param cards Card rewards
     * @param relics Relic rewards
     */
    void showRewards(int gold, 
                     const std::vector<Card*>& cards,
                     const std::vector<Relic*>& relics) override;
    
    /**
     * @brief Display game over screen
     * @param victory Whether player won
     * @param score Final score
     */
    void showGameOver(bool victory, int score) override;

    /**
     * @brief Show an event
     * @param event The event to display
     * @param player The current player
     */
    void showEvent(const Event* event, const Player* player) override;

    /**
     * @brief Show an event result
     * @param resultText The text to display as a result of the event
     */
    void showEventResult(const std::string& resultText) override;

    /**
     * @brief Prints a line of equals signs (useful for section dividers)
     * @param width Width of the divider (default: 80)
     */
    void printDivider(int width = 80);

private:
    Game* game_ = nullptr;                     ///< Pointer to the game instance
    std::function<bool(const std::string&)> inputCallback_; ///< Input callback function
    
    std::thread inputThread_;                  ///< Thread for handling input
    std::atomic<bool> running_;                ///< Whether the UI is running
    std::mutex inputMutex_;                    ///< Mutex for input queue
    std::condition_variable inputCondition_;   ///< Condition variable for input queue
    std::queue<std::string> inputQueue_;       ///< Queue of input strings
    
    /**
     * @brief Input thread function
     */
    void inputThreadFunc();
    
    /**
     * @brief Process pending input
     */
    void processInput();
    
    /**
     * @brief Get room type as string
     * @param type Room type enum
     * @return String representation
     */
    std::string getRoomTypeString(RoomType type) const;
    
    /**
     * @brief Get card type as string
     * @param type Card type enum
     * @return String representation
     */
    std::string getCardTypeString(CardType type) const;
    
    /**
     * @brief Get card rarity as string
     * @param rarity Card rarity enum
     * @return String representation
     */
    std::string getCardRarityString(CardRarity rarity) const;
    
    /**
     * @brief Get relic rarity as string
     * @param rarity Relic rarity enum
     * @return String representation
     */
    std::string getRelicRarityString(RelicRarity rarity) const;
    
    /**
     * @brief Center a string in a field
     * @param str String to center
     * @param width Field width
     * @return Centered string
     */
    std::string centerString(const std::string& str, int width = 80) const;

    /**
     * @brief Show help for map commands
     */
    void showMapHelp();

    /**
     * @brief Show help for combat commands
     */
    void showCombatHelp();

    /**
     * @brief Show help for event commands
     */
    void showEventHelp();

    // Last message displayed by this UI
    std::string lastMessage;
    std::string lastInputPrompt;
};

} // namespace deckstiny 