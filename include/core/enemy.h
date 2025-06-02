#pragma once

#include "core/character.h"
#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

namespace deckstiny {

// Forward declarations
class Combat;
class Player;

/**
 * @struct Intent
 * @brief Structure representing an enemy's intent
 */
struct Intent {
    std::string type;                           ///< Type of intent (attack, defend, buff, debuff, etc.)
    int value = 0;                              ///< Primary value (damage, block, etc.)
    int secondaryValue = 0;                     ///< Secondary value (if needed)
    std::string target;                         ///< Target of the intent (player, self, ally, etc.)
    std::string effect;                         ///< Additional effect (if applicable)
    nlohmann::json associatedEffectsJson;       ///< Raw JSON of the full effects array for this move
};

/**
 * @class Enemy
 * @brief Represents an enemy character in the game
 * 
 * Extends Character with enemy-specific functionality like
 * AI logic, intents, rewards, etc.
 */
class Enemy : public Character {
public:
    /**
     * @brief Default constructor
     */
    Enemy() = default;
    
    /**
     * @brief Constructor with enemy parameters
     * @param id Unique identifier
     * @param name Display name
     * @param maxHealth Maximum health points
     */
    Enemy(const std::string& id, const std::string& name, int maxHealth);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Enemy() = default;
    
    /**
     * @brief Get the enemy's current intent
     * @return Current intent
     */
    const Intent& getIntent() const;
    
    /**
     * @brief Set the enemy's intent
     * @param intent New intent
     */
    void setIntent(const Intent& intent);
    
    /**
     * @brief Check if the enemy is elite
     * @return True if elite, false otherwise
     */
    bool isElite() const;
    
    /**
     * @brief Set the enemy's elite status
     * @param elite New elite status
     */
    void setElite(bool elite);
    
    /**
     * @brief Check if the enemy is a boss
     * @return True if boss, false otherwise
     */
    bool isBoss() const;
    
    /**
     * @brief Set the enemy's boss status
     * @param boss New boss status
     */
    void setBoss(bool boss);
    
    /**
     * @brief Get minimum gold reward
     * @return Minimum gold
     */
    int getMinGold() const;
    
    /**
     * @brief Get maximum gold reward
     * @return Maximum gold
     */
    int getMaxGold() const;
    
    /**
     * @brief Set gold reward range
     * @param min Minimum gold
     * @param max Maximum gold
     */
    void setGoldReward(int min, int max);
    
    /**
     * @brief Get gold reward for defeating this enemy
     * @return Random gold amount within range
     */
    int rollGoldReward() const;
    
    /**
     * @brief Choose and set the next move
     * @param combat Current combat instance
     * @param player Player character
     */
    virtual void chooseNextMove(Combat* combat, Player* player);
    
    /**
     * @brief Execute the enemy's turn
     * @param combat Current combat instance
     * @param player Player character
     */
    virtual void takeTurn(Combat* combat, Player* player);
    
    /**
     * @brief Get all possible moves for this enemy
     * @return Vector of possible move IDs
     */
    const std::vector<std::string>& getPossibleMoves() const;
    
    /**
     * @brief Add a possible move
     * @param moveId ID of the move to add
     */
    void addPossibleMove(const std::string& moveId);
    
    /**
     * @brief Start of turn processing
     */
    void startTurn() override;
    
    /**
     * @brief End of turn processing
     */
    void endTurn() override;
    
    /**
     * @brief Load enemy data from JSON
     * @param json JSON object containing enemy data
     * @return True if loading was successful, false otherwise
     */
    bool loadFromJson(const nlohmann::json& json) override;
    
    /**
     * @brief Create a clone of this enemy
     * @return Unique pointer to a new enemy with the same properties
     */
    std::unique_ptr<Entity> clone() const override;

    /**
     * @brief Create a shared pointer clone of this enemy
     * @return Shared pointer to a new enemy with the same properties
     */
    std::shared_ptr<Enemy> cloneEnemy() const;
    
private:
    Intent currentIntent_;                                 ///< Current intent
    std::vector<std::string> moves_;                       ///< Possible moves
    std::unordered_map<std::string, Intent> moveIntents_;  ///< Map of move IDs to their intent data
    bool elite_ = false;                                   ///< Whether this is an elite enemy
    bool boss_ = false;                                    ///< Whether this is a boss enemy
    int minGold_ = 10;                                     ///< Minimum gold reward
    int maxGold_ = 20;                                     ///< Maximum gold reward
};

} // namespace deckstiny 