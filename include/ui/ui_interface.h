// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#ifndef DECKSTINY_UI_INTERFACE_H
#define DECKSTINY_UI_INTERFACE_H

#include <string>
#include <vector>
#include <functional>
#include <map>

namespace deckstiny {

// Forward declarations
class Game;
class Player;
class Enemy;
class Combat;
class Card;
class Relic;
struct Room;
class Event;

/**
 * @class UIInterface
 * @brief Abstract interface for UI implementations
 * 
 * This interface defines the methods that any UI
 * implementation (text or graphical) must provide.
 */
class UIInterface {
public:
    /**
     * @brief Virtual destructor
     */
    virtual ~UIInterface() = default;
    
    /**
     * @brief Initialize the UI
     * @param game Pointer to the game instance
     * @return True if initialization succeeded, false otherwise
     */
    virtual bool initialize(Game* game) = 0;
    
    /**
     * @brief Run the UI main loop
     */
    virtual void run() = 0;
    
    /**
     * @brief Shut down the UI
     */
    virtual void shutdown() = 0;
    
    /**
     * @brief Set input callback
     * @param callback Function to call when input is received
     */
    virtual void setInputCallback(std::function<bool(const std::string&)> callback) = 0;
    
    /**
     * @brief Display the main menu
     */
    virtual void showMainMenu() = 0;
    
    /**
     * @brief Display the character selection screen
     * @param availableClasses List of available character classes
     */
    virtual void showCharacterSelection(const std::vector<std::string>& availableClasses) = 0;
    
    /**
     * @brief Show the game map
     * @param currentRoomId ID of the current room
     * @param availableRooms List of available room IDs
     * @param allRooms Map of all rooms
     */
    virtual void showMap(int currentRoomId,
                         const std::vector<int>& availableRooms,
                         const std::unordered_map<int, Room>& allRooms) = 0;
    
    /**
     * @brief Display combat state
     * @param combat Current combat instance
     */
    virtual void showCombat(const Combat* combat) = 0;
    
    /**
     * @brief Display player stats
     * @param player Player character
     */
    virtual void showPlayerStats(const Player* player) = 0;
    
    /**
     * @brief Display enemy stats
     * @param enemy Enemy character
     */
    virtual void showEnemyStats(const Enemy* enemy) = 0;
    
    /**
     * @brief Display an enemy selection menu
     * @param combat Current combat instance
     * @param cardName Name of the card being played
     */
    virtual void showEnemySelectionMenu(const Combat* combat, const std::string& cardName) = 0;
    
    /**
     * @brief Display a card
     * @param card Card to display
     * @param showEnergyCost Whether to show energy cost
     * @param selected Whether the card is selected
     */
    virtual void showCard(const Card* card, bool showEnergyCost = true, bool selected = false) = 0;
    
    /**
     * @brief Display a list of cards
     * @param cards Cards to display
     * @param title Optional title for the card list
     * @param showIndices Whether to show indices for selection
     */
    virtual void showCards(const std::vector<Card*>& cards, 
                           const std::string& title = "",
                           bool showIndices = true) = 0;
    
    /**
     * @brief Display a relic
     * @param relic Relic to display
     */
    virtual void showRelic(const Relic* relic) = 0;
    
    /**
     * @brief Display a list of relics
     * @param relics Relics to display
     * @param title Optional title for the relic list
     */
    virtual void showRelics(const std::vector<Relic*>& relics,
                            const std::string& title = "") = 0;
    
    /**
     * @brief Display a message
     * @param message Message to display
     * @param pause Whether to pause for user acknowledgement
     */
    virtual void showMessage(const std::string& message, bool pause = false) = 0;
    
    /**
     * @brief Display a prompt and get input
     * @param prompt Prompt to display
     * @return User input string
     */
    virtual std::string getInput(const std::string& prompt) = 0;
    
    /**
     * @brief Clear the screen
     */
    virtual void clearScreen() const = 0;
    
    /**
     * @brief Update the display
     */
    virtual void update() = 0;
    
    /**
     * @brief Display rewards after combat
     * @param gold Gold reward
     * @param cards Card rewards
     * @param relics Relic rewards
     */
    virtual void showRewards(int gold, 
                             const std::vector<Card*>& cards,
                             const std::vector<Relic*>& relics) = 0;
    
    /**
     * @brief Display game over screen
     * @param victory Whether player won
     * @param score Final score
     */
    virtual void showGameOver(bool victory, int score) = 0;

    /**
     * @brief Show an event
     * @param event The event to display
     * @param player The current player
     */
    virtual void showEvent(const Event* event, const Player* player) = 0;

    /**
     * @brief Show an event result
     * @param resultText The text to display as a result of the event
     */
    virtual void showEventResult(const std::string& resultText) = 0;

    /**
     * @brief Display the shop interface
     * @param cardsForSale List of cards available for purchase
     * @param relicsForSale List of relics available for purchase
     * @param playerGold Current gold of the player
     */
    virtual void showShop(const std::vector<Card*>& cardsForSale,
                          const std::vector<Relic*>& relicsForSale,
                          int playerGold) = 0;

    /**
     * @brief Show the shop UI with relic prices
     * @param cards Cards for sale
     * @param relics Relics for sale
     * @param relicPrices Map of relics to their prices
     * @param playerGold Current player gold
     */
    virtual void showShop(const std::vector<Card*>& cards, 
                         const std::vector<Relic*>& relics,
                         const std::unordered_map<Relic*, int>& relicPrices,
                         int playerGold) {
        // Prevent unused parameter warning
        (void)relicPrices;
        
        // Default implementation just calls the regular showShop
        showShop(cards, relics, playerGold);
    }
};

} // namespace deckstiny 

#endif // DECKSTINY_UI_INTERFACE_H