#pragma once

#include "core/entity.h"
#include <vector>
#include <unordered_map>

namespace deckstiny {

/**
 * @class Character
 * @brief Base class for all characters (player and enemies)
 * 
 * This class extends Entity to include health, energy, and other stats
 * common to all characters in the game.
 */
class Character : public Entity {
public:
    /**
     * @brief Default constructor
     */
    Character() = default;
    
    /**
     * @brief Constructor with id, name, health, and energy
     * @param id Unique identifier for the character
     * @param name Display name of the character
     * @param maxHealth Maximum health points
     * @param baseEnergy Base energy points per turn
     */
    Character(const std::string& id, const std::string& name, int maxHealth, int baseEnergy);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Character() = default;
    
    /**
     * @brief Get current health
     * @return Current health points
     */
    int getHealth() const;
    
    /**
     * @brief Set current health
     * @param health New health value (capped at max health)
     */
    void setHealth(int health);
    
    /**
     * @brief Get maximum health
     * @return Maximum health points
     */
    int getMaxHealth() const;
    
    /**
     * @brief Set maximum health
     * @param maxHealth New maximum health value
     */
    void setMaxHealth(int maxHealth);
    
    /**
     * @brief Check if character is alive
     * @return True if health > 0, false otherwise
     */
    bool isAlive() const;
    
    /**
     * @brief Apply damage to the character
     * @param amount Amount of damage to apply
     * @return Actual damage taken (may be reduced by block)
     */
    virtual int takeDamage(int amount);
    
    /**
     * @brief Heal the character
     * @param amount Amount of healing to apply
     * @return Actual healing received
     */
    virtual int heal(int amount);
    
    /**
     * @brief Add block to the character
     * @param amount Amount of block to add
     */
    virtual void addBlock(int amount);
    
    /**
     * @brief Get current block amount
     * @return Current block points
     */
    int getBlock() const;
    
    /**
     * @brief Get base energy per turn
     * @return Base energy points
     */
    int getBaseEnergy() const;
    
    /**
     * @brief Get current energy
     * @return Current energy points
     */
    int getEnergy() const;
    
    /**
     * @brief Set current energy
     * @param energy New energy value
     */
    void setEnergy(int energy);
    
    /**
     * @brief Use energy
     * @param amount Amount of energy to use
     * @return True if successful, false if insufficient energy
     */
    bool useEnergy(int amount);
    
    /**
     * @brief Reset energy to base value (used at start of turn)
     */
    void resetEnergy();
    
    /**
     * @brief Reset block to zero (used at end of turn)
     */
    void resetBlock();
    
    /**
     * @brief Add a status effect
     * @param effect Name of the effect
     * @param stacks Number of stacks to add
     */
    void addStatusEffect(const std::string& effect, int stacks);
    
    /**
     * @brief Get status effect stacks
     * @param effect Name of the effect
     * @return Number of stacks, 0 if effect not present
     */
    int getStatusEffect(const std::string& effect) const;
    
    /**
     * @brief Check if has a specific status effect
     * @param effect Name of the effect
     * @return True if effect is present, false otherwise
     */
    bool hasStatusEffect(const std::string& effect) const;
    
    /**
     * @brief Get all status effects
     * @return Map of effect names to stack counts
     */
    const std::unordered_map<std::string, int>& getStatusEffects() const;
    
    /**
     * @brief Start of turn processing
     */
    virtual void startTurn();
    
    /**
     * @brief End of turn processing
     */
    virtual void endTurn();
    
    /**
     * @brief Load character data from JSON
     * @param json JSON object containing character data
     * @return True if loading was successful, false otherwise
     */
    bool loadFromJson(const nlohmann::json& json) override;
    
    /**
     * @brief Create a clone of this character
     * @return Unique pointer to a new character with the same properties
     */
    std::unique_ptr<Entity> clone() const override;

private:
    int maxHealth_ = 0;     ///< Maximum health points
    int currentHealth_ = 0; ///< Current health points
    int block_ = 0;         ///< Current block points
    int baseEnergy_ = 0;    ///< Base energy per turn
    int currentEnergy_ = 0; ///< Current energy points
    
    /// Status effects and their stack counts
    std::unordered_map<std::string, int> statusEffects_;
};

} // namespace deckstiny 