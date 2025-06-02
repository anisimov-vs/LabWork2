// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <functional>
#include <random> // Required for std::mt19937
#include <map> // Required for std::map

namespace deckstiny {

// Forward declarations
class Player;
class Combat;
class Card;
class Enemy;
class Relic;
class GameMap;
class UIInterface;
class Event;

/**
 * @struct CharacterData
 * @brief Holds the data for a character class, loaded from JSON.
 */
struct CharacterData {
    std::string id;
    std::string name;
    int max_health;
    int base_energy;
    int initial_hand_size;
    std::string description;
    std::vector<std::string> starting_deck;
    std::vector<std::string> starting_relics;
    // PlayerClass class_enum; // May need to map string to enum if PlayerClass is used directly
};

/**
 * @enum GameState
 * @brief Represents the current state of the game
 */
enum class GameState {
    MAIN_MENU,
    CHARACTER_SELECT,
    MAP,
    COMBAT,
    EVENT,
    SHOP,
    REWARD,
    CARD_SELECT,
    REST,
    GAME_OVER
};

/**
 * @class Game
 * @brief Main game manager class
 * 
 * Manages the overall game state, progression, and
 * interaction between various game systems.
 */
class Game {
public:
    /**
     * @brief Default constructor
     */
    Game();
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Game();
    
    /**
     * @brief Initialize the game
     * @param uiInterface Interface to use for UI
     * @return True if initialization succeeded, false otherwise
     */
    bool initialize(std::shared_ptr<UIInterface> uiInterface);
    
    /**
     * @brief Run the game main loop
     */
    void run();
    
    /**
     * @brief Shut down the game
     */
    void shutdown();
    
    /**
     * @brief Get the current game state
     * @return Current game state
     */
    GameState getState() const { return state_; }
    
    /**
     * @brief Check if the game is running
     * @return True if the game is running, false otherwise
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief Change the game state
     * @param state New game state
     */
    void setState(GameState state);
    
    /**
     * @brief Get the current player
     * @return Pointer to the player, or nullptr if no player exists
     */
    Player* getPlayer() { return player_.get(); }
    
    /**
     * @brief Create a new player
     * @param className Name of the player class to create
     * @param playerName Custom player name (optional)
     * @return True if player creation succeeded, false otherwise
     */
    bool createPlayer(const std::string& className, const std::string& playerName = "");
    
    /**
     * @brief Get the current combat
     * @return Pointer to the current combat, nullptr if not in combat
     */
    Combat* getCurrentCombat() const;
    
    /**
     * @brief Start a new combat
     * @param enemies List of enemy IDs to include in combat
     * @return True if combat setup succeeded, false otherwise
     */
    bool startCombat(const std::vector<std::string>& enemies);
    
    /**
     * @brief End the current combat
     * @param victorious Whether player won the combat
     */
    void endCombat(bool victorious);
    
    /**
     * @brief Get the current map
     * @return Pointer to the game map
     */
    GameMap* getMap() const;
    
    /**
     * @brief Generate a new game map
     * @param act Current act number
     * @return True if map generation succeeded, false otherwise
     */
    bool generateMap(int act);
    
    /**
     * @brief Load a card template
     * @param id Card ID to load
     * @return Shared pointer to the loaded card, nullptr if loading failed
     */
    std::shared_ptr<Card> loadCard(const std::string& id);
    
    /**
     * @brief Load all card templates
     * @return True if loading succeeded, false otherwise
     */
    bool loadAllCards();
    
    /**
     * @brief Load an enemy template
     * @param id Enemy ID to load
     * @return Shared pointer to the loaded enemy, nullptr if loading failed
     */
    std::shared_ptr<Enemy> loadEnemy(const std::string& id);
    
    /**
     * @brief Load all enemy templates
     * @return True if loading succeeded, false otherwise
     */
    bool loadAllEnemies();
    
    /**
     * @brief Load a relic template
     * @param id Relic ID to load
     * @return Shared pointer to the loaded relic, nullptr if loading failed
     */
    std::shared_ptr<Relic> loadRelic(const std::string& id);
    
    /**
     * @brief Load all relic templates
     * @return True if loading succeeded, false otherwise
     */
    bool loadAllRelics();
    
    /**
     * @brief Get UI interface
     * @return Pointer to the UI interface
     */
    UIInterface* getUI() const;
    
    /**
     * @brief Process input for the current state
     * @param input Input string
     * @return True if input was handled, false otherwise
     */
    bool processInput(const std::string& input);
    
    /**
     * @brief Add a card to the player's deck
     * @param cardId ID of the card to add
     * @return True if card was added successfully, false otherwise
     */
    bool addCardToDeck(const std::string& cardId);
    
    /**
     * @brief Add a relic to the player
     * @param relicId ID of the relic to add
     * @return True if relic was added successfully, false otherwise
     */
    bool addRelic(const std::string& relicId);

    /**
     * @brief Show UI for selecting and upgrading a card from the player's deck
     * @return Name of the upgraded card, or error message if upgrade failed
     */
    std::string upgradeCard();

    /**
     * @brief Get current game state
     * @return Current game state
     */
    GameState getCurrentState() const { return state_; }
    
    /**
     * @brief Calculate the player's score based on their progress
     * @return The calculated score
     */
    int calculateScore() const;

    // Add getters for loaded data for testing and other purposes
    const std::unordered_map<std::string, std::shared_ptr<Card>>& getAllCards() const { return allCards_; }
    std::shared_ptr<Card> getCardData(const std::string& id) const;

    const std::unordered_map<std::string, std::shared_ptr<Enemy>>& getAllEnemies() const { return allEnemies_; }
    std::shared_ptr<Enemy> getEnemyData(const std::string& id) const;

    const std::unordered_map<std::string, std::shared_ptr<Relic>>& getAllRelics() const { return allRelics_; }
    std::shared_ptr<Relic> getRelicData(const std::string& id) const;
    
    const std::unordered_map<std::string, std::shared_ptr<Event>>& getAllEvents() const { return allEvents_; }
    std::shared_ptr<Event> getEventData(const std::string& id) const;

    /**
     * @brief Get a random card from the master list, optionally filtered by rarity.
     * @param rarity_filter Optional rarity to filter by. If empty, any rarity can be chosen.
     * @return Shared pointer to a random card, or nullptr if no card matches.
     */
    std::shared_ptr<Card> getRandomCardFromMasterList(const std::string& rarity_filter = "");

    /**
     * @brief Get a random relic from the master list.
     * @return Shared pointer to a random relic, or nullptr if no relics are loaded.
     */
    std::shared_ptr<Relic> getRandomRelicFromMasterList();

    // Getter for all loaded character data
    const std::map<std::string, CharacterData>& getAllCharacterData() const { return allCharacters_; }

private:
    std::shared_ptr<UIInterface> ui_;                  ///< User interface
    std::shared_ptr<Player> player_;                   ///< Player character
    std::unique_ptr<Combat> currentCombat_;            ///< Current combat
    std::unique_ptr<GameMap> map_;                     ///< Game map
    std::shared_ptr<Event> currentEvent_;              ///< Current event (if in event state)
    bool running_ = false;                             ///< Whether the game is running
    GameState state_ = GameState::MAIN_MENU;           ///< Current game state
    
    // Data stores for game assets
    std::unordered_map<std::string, std::shared_ptr<Card>> allCards_;
    std::unordered_map<std::string, std::shared_ptr<Enemy>> allEnemies_;
    std::unordered_map<std::string, std::shared_ptr<Relic>> allRelics_;
    std::unordered_map<std::string, std::shared_ptr<Event>> allEvents_;
    std::map<std::string, CharacterData> allCharacters_; // Stores loaded character data

    int failedLoads_ = 0; // Counter for failed data loads (used by multiple loaders)
    
    // Card selection state for two-step targeting
    bool awaitingEnemySelection_ = false;              ///< Whether we're waiting for enemy selection
    int selectedCardIndex_ = -1;                       ///< Index of the selected card
    bool transitioningFromCombat_ = false;             ///< Whether we're transitioning from combat to rewards
    
    bool developerMode_; // From config, determines if dev features are available at all
    bool developerModeEnabled_ = false; // Runtime toggle for active developer features; initialize to false
    
    std::unordered_map<GameState, std::function<bool(const std::string&)>> inputHandlers_;  ///< Input handlers
    
    // Input handlers
    bool handleMainMenuInput(const std::string& input);
    bool handleCharacterSelectInput(const std::string& input);
    bool handleMapInput(const std::string& input);
    bool handleCombatInput(const std::string& input);
    bool handleEventInput(const std::string& input);
    bool handleShopInput(const std::string& input);
    
    /**
     * @brief Initialize the logging system
     */
    void initializeLogging();
    
    /**
     * @brief Initialize input handlers
     */
    void initializeInputHandlers();
    
    /**
     * @brief Load data from JSON files
     * @return True if loading succeeded, false otherwise
     */
    bool loadGameData();
    
    /**
     * @brief Load an event template
     * @param id Event ID to load
     * @return Shared pointer to the loaded event, nullptr if loading failed
     */
    std::shared_ptr<Event> loadEvent(const std::string& id);
    
    /**
     * @brief Load all event templates
     * @return True if loading succeeded, false otherwise
     */
    bool loadAllEvents();
    
    /**
     * @brief Start an event
     * @param eventId ID of the event to start
     * @return True if event started successfully, false otherwise
     */
    bool startEvent(const std::string& eventId);
    
    /**
     * @brief End the current event
     */
    void endEvent();
    
    void startShop(); // Method to initialize shop inventory

    std::vector<Card*> shopCardsForSale_; // Added for shop inventory
    std::vector<Relic*> shopRelicsForSale_; // Added for shop inventory
    std::unordered_map<Relic*, int> shopRelicPrices_; // Map to store relic prices

    std::mt19937 rng_; // Random number generator

    // Specific data loaders
    bool loadAllCharacters();
};

} // namespace deckstiny 