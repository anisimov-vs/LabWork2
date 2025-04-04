#pragma once

#include "core/entity.h"
#include <memory>

namespace deckstiny {

// Forward declarations
class Player;
class Combat;

/**
 * @enum RelicRarity
 * @brief Represents the rarity of a relic
 */
enum class RelicRarity {
    STARTER,
    COMMON,
    UNCOMMON,
    RARE,
    BOSS,
    SHOP,
    EVENT
};

/**
 * @class Relic
 * @brief Represents a relic in the game
 * 
 * Relics provide passive bonuses or special abilities
 * to the player throughout a run.
 */
class Relic : public Entity {
public:
    /**
     * @brief Default constructor
     */
    Relic() = default;
    
    /**
     * @brief Constructor with relic properties
     * @param id Unique identifier
     * @param name Display name
     * @param description Relic description text
     * @param rarity Relic rarity
     * @param flavorText Optional flavor text
     */
    Relic(const std::string& id, const std::string& name, const std::string& description,
          RelicRarity rarity, const std::string& flavorText = "");
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Relic() = default;
    
    /**
     * @brief Get the relic's description
     * @return String containing relic description
     */
    const std::string& getDescription() const;
    
    /**
     * @brief Set the relic's description
     * @param description New description text
     */
    void setDescription(const std::string& description);
    
    /**
     * @brief Get the relic's flavor text
     * @return String containing flavor text
     */
    const std::string& getFlavorText() const;
    
    /**
     * @brief Set the relic's flavor text
     * @param flavorText New flavor text
     */
    void setFlavorText(const std::string& flavorText);
    
    /**
     * @brief Get the relic's rarity
     * @return Relic rarity enum
     */
    RelicRarity getRarity() const;
    
    /**
     * @brief Get the relic's counter value
     * @return Current counter value
     */
    int getCounter() const;
    
    /**
     * @brief Set the relic's counter value
     * @param counter New counter value
     */
    void setCounter(int counter);
    
    /**
     * @brief Increment the relic's counter
     * @param amount Amount to increment
     * @return New counter value
     */
    int incrementCounter(int amount = 1);
    
    /**
     * @brief Reset the relic's counter
     */
    void resetCounter();
    
    /**
     * @brief Called when the relic is obtained
     * @param player Player who obtained the relic
     */
    virtual void onObtain(Player* player);
    
    /**
     * @brief Called at the start of combat
     * @param player Player with the relic
     * @param combat Current combat instance
     */
    virtual void onCombatStart(Player* player, Combat* combat);
    
    /**
     * @brief Called at the start of player's turn
     * @param player Player with the relic
     * @param combat Current combat instance
     */
    virtual void onTurnStart(Player* player, Combat* combat);
    
    /**
     * @brief Called at the end of player's turn
     * @param player Player with the relic
     * @param combat Current combat instance
     */
    virtual void onTurnEnd(Player* player, Combat* combat);
    
    /**
     * @brief Called when player takes damage
     * @param player Player with the relic
     * @param damage Amount of damage taken
     * @param combat Current combat instance
     * @return Modified damage
     */
    virtual int onTakeDamage(Player* player, int damage, Combat* combat);
    
    /**
     * @brief Called when player deals damage
     * @param player Player with the relic
     * @param damage Amount of damage dealt
     * @param targetIndex Index of the target
     * @param combat Current combat instance
     * @return Modified damage
     */
    virtual int onDealDamage(Player* player, int damage, int targetIndex, Combat* combat);
    
    /**
     * @brief Called at the end of combat
     * @param player Player with the relic
     * @param victorious Whether player won the combat
     * @param combat Current combat instance
     */
    virtual void onCombatEnd(Player* player, bool victorious, Combat* combat);
    
    /**
     * @brief Load relic data from JSON
     * @param json JSON object containing relic data
     * @return True if loading was successful, false otherwise
     */
    bool loadFromJson(const nlohmann::json& json) override;
    
    /**
     * @brief Create a clone of this relic
     * @return Unique pointer to a new relic with the same properties
     */
    std::unique_ptr<Entity> clone() const override;
    
    /**
     * @brief Create a shared pointer clone of this relic
     * @return Shared pointer to a new relic with the same properties
     */
    virtual std::shared_ptr<Relic> cloneRelic() const;

private:
    std::string description_;              ///< Relic description text
    std::string flavorText_;               ///< Relic flavor text
    RelicRarity rarity_ = RelicRarity::COMMON; ///< Relic rarity
    int counter_ = 0;                      ///< Relic counter (for tracking purposes)
};

} // namespace deckstiny 