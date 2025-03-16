#pragma once

#include <vector>
#include <memory>
#include <functional>
#include <queue>
#include <string>

namespace deckstiny {

// Forward declarations
class Player;
class Enemy;
class Card;
class Game;

/**
 * @struct CombatAction
 * @brief Structure representing a delayed combat action
 */
struct CombatAction {
    std::function<void()> action;      ///< Action to execute
    int priority = 0;                  ///< Action priority (higher = earlier)
    int delay = 0;                     ///< Delay in turns
    std::string source;                ///< Source of the action (for tracking)
};

/**
 * @class Combat
 * @brief Manages combat between the player and enemies
 * 
 * Handles turn processing, action resolution, and combat state.
 */
class Combat {
public:
    /**
     * @brief Default constructor
     */
    Combat();
    
    /**
     * @brief Constructor with player
     * @param player Pointer to the player character
     */
    explicit Combat(Player* player);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Combat() = default;
    
    /**
     * @brief Set the player character
     * @param player Pointer to the player character
     */
    void setPlayer(Player* player);
    
    /**
     * @brief Get the player character
     * @return Pointer to the player character
     */
    Player* getPlayer() const;
    
    /**
     * @brief Set the game instance
     * @param game Pointer to the game instance
     */
    void setGame(Game* game);
    
    /**
     * @brief Get the game instance
     * @return Pointer to the game instance
     */
    Game* getGame() const;
    
    /**
     * @brief Add an enemy to the combat
     * @param enemy Shared pointer to the enemy to add
     */
    void addEnemy(std::shared_ptr<Enemy> enemy);
    
    /**
     * @brief Get all enemies
     * @return Vector of enemy pointers
     */
    const std::vector<std::shared_ptr<Enemy>>& getEnemies() const;
    
    /**
     * @brief Get number of enemies
     * @return Number of enemies
     */
    size_t getEnemyCount() const;
    
    /**
     * @brief Get enemy at index
     * @param index Index of the enemy
     * @return Pointer to the enemy, nullptr if invalid index
     */
    Enemy* getEnemy(size_t index) const;
    
    /**
     * @brief Check if all enemies are defeated
     * @return True if all enemies are defeated, false otherwise
     */
    bool areAllEnemiesDefeated() const;
    
    /**
     * @brief Check if player is defeated
     * @return True if player is defeated, false otherwise
     */
    bool isPlayerDefeated() const;
    
    /**
     * @brief Check if combat is over
     * @return True if combat is over, false otherwise
     */
    bool isCombatOver() const;
    
    /**
     * @brief Get current turn number
     * @return Current turn
     */
    int getTurn() const;
    
    /**
     * @brief Get whether it's currently the player's turn
     * @return True if player's turn, false otherwise
     */
    bool isPlayerTurn() const;
    
    /**
     * @brief Start the combat
     */
    void start();
    
    /**
     * @brief Begin player turn
     */
    void beginPlayerTurn();
    
    /**
     * @brief End player turn
     */
    void endPlayerTurn();
    
    /**
     * @brief Process enemy turns
     */
    void processEnemyTurns();
    
    /**
     * @brief Play a card
     * @param cardIndex Index of the card in player's hand
     * @param targetIndex Index of the target (if applicable)
     * @return True if card played successfully, false otherwise
     */
    bool playCard(int cardIndex, int targetIndex = -1);
    
    /**
     * @brief Add a delayed action
     * @param action Function to execute
     * @param delay Delay in turns
     * @param priority Action priority
     * @param source Source description
     */
    void addDelayedAction(std::function<void()> action, int delay = 0, 
                          int priority = 0, const std::string& source = std::string());
    
    /**
     * @brief Process delayed actions for current turn
     */
    void processDelayedActions();
    
    /**
     * @brief Handle enemy death
     * @param index Index of the dead enemy
     */
    void handleEnemyDeath(size_t index);
    
    /**
     * @brief Calculate rewards for combat
     */
    void calculateRewards();
    
    /**
     * @brief End the combat
     * @param victorious Whether player was victorious
     */
    void end(bool victorious);

private:
    Player* player_ = nullptr;                           ///< Player character
    Game* game_ = nullptr;                               ///< Game instance
    std::vector<std::shared_ptr<Enemy>> enemies_;        ///< Enemy characters
    int turn_ = 0;                                       ///< Current turn number
    bool playerTurn_ = true;                             ///< Whether it's player's turn
    bool inCombat_ = false;                              ///< Whether combat is active
    
    /**
     * @brief Comparator for combat actions
     */
    struct ActionComparator {
        bool operator()(const CombatAction& a, const CombatAction& b) const {
            if (a.delay != b.delay) {
                return a.delay > b.delay;
            }
            return a.priority < b.priority;
        }
    };
    
    /// Priority queue for delayed actions
    std::priority_queue<CombatAction, std::vector<CombatAction>, ActionComparator> delayedActions_;
};

} // namespace deckstiny 