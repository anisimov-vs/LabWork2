// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#ifndef DECKSTINY_CORE_CARD_H
#define DECKSTINY_CORE_CARD_H

#include "core/entity.h"
#include <memory>
#include <functional>

namespace deckstiny {

class Character;
class Player;
class Combat;

/**
 * @enum CardType
 * @brief Represents the type of a card
 */
enum class CardType {
    ATTACK,
    SKILL,
    POWER,
    STATUS,
    CURSE
};

/**
 * @enum CardRarity
 * @brief Represents the rarity of a card
 */
enum class CardRarity {
    COMMON,
    UNCOMMON,
    RARE,
    SPECIAL,
    BASIC
};

/**
 * @enum CardTarget
 * @brief Represents the targeting behavior of a card
 */
enum class CardTarget {
    NONE,
    SELF,
    SINGLE_ENEMY,
    ALL_ENEMIES,
    SINGLE_ALLY,
    ALL_ALLIES
};

/**
 * @class Card
 * @brief Represents a card in the game
 * 
 * Cards are the primary gameplay mechanic, allowing players
 * to perform actions during combat.
 */
class Card : public Entity {
public:
    /**
     * @brief Default constructor
     */
    Card() = default;
    
    /**
     * @brief Constructor with card properties
     * @param id Unique identifier
     * @param name Display name
     * @param description Card description text
     * @param type Card type
     * @param rarity Card rarity
     * @param target Card target type
     * @param cost Energy cost to play
     * @param upgradable Whether the card can be upgraded
     */
    Card(const std::string& id, const std::string& name, const std::string& description,
         CardType type, CardRarity rarity, CardTarget target, int cost, bool upgradable);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Card() = default;
    
    /**
     * @brief Get the card's description
     * @return String containing card description
     */
    const std::string& getDescription() const;
    
    /**
     * @brief Set the card's description
     * @param description New description text
     */
    void setDescription(const std::string& description);
    
    /**
     * @brief Get the card's type
     * @return Card type enum
     */
    CardType getType() const;
    
    /**
     * @brief Get the card's rarity
     * @return Card rarity enum
     */
    CardRarity getRarity() const;
    
    /**
     * @brief Get the card's target type
     * @return Card target enum
     */
    CardTarget getTarget() const;
    
    /**
     * @brief Get the card's energy cost
     * @return Energy cost
     */
    int getCost() const;
    
    /**
     * @brief Set the card's energy cost
     * @param cost New energy cost
     */
    void setCost(int cost);
    
    /**
     * @brief Check if card is upgradable
     * @return True if upgradable, false otherwise
     */
    bool isUpgradable() const;
    
    /**
     * @brief Check if card is upgraded
     * @return True if upgraded, false otherwise
     */
    bool isUpgraded() const;
    
    /**
     * @brief Upgrade the card
     * @return True if successful, false if already upgraded or not upgradable
     */
    virtual bool upgrade();
    
    /**
     * @brief Check if card can be played in the current state
     * @param player Player character
     * @param targetIndex Index of the target (if needed)
     * @param combat Current combat state
     * @return True if can be played, false otherwise
     */
    virtual bool canPlay(Player* player, int targetIndex = -1, Combat* combat = nullptr) const;
    
    /**
     * @brief Check if this card needs a target
     * @return True if the card needs a target, false otherwise
     */
    virtual bool needsTarget() const;
    
    /**
     * @brief Play the card
     * @param player Player playing the card
     * @param targetIndex Index of the target (if applicable)
     * @param combat Current combat instance
     * @return True if successfully played, false otherwise
     */
    virtual bool play(Player* player, int targetIndex, Combat* combat);
    
    /**
     * @brief Load card data from JSON
     * @param json JSON object containing card data
     * @return True if loading was successful, false otherwise
     */
    bool loadFromJson(const nlohmann::json& json) override;
    
    /**
     * @brief Create a clone of this card
     * @return Unique pointer to a new card with the same properties
     */
    std::unique_ptr<Entity> clone() const override;
    
    /**
     * @brief Create a shared pointer clone of this card
     * @return Shared pointer to a new card with the same properties
     */
    virtual std::shared_ptr<Card> cloneCard() const;

    /**
     * @brief Get the card's class restriction
     * @return String containing the class restriction, or empty string if none
     */
    const std::string& getClassRestriction() const;
    
    /**
     * @brief Set the card's class restriction
     * @param className Class name to restrict the card to
     */
    void setClassRestriction(const std::string& className);
    
    /**
     * @brief Check if a player can use this card
     * @param player Player to check
     * @return True if the player can use this card, false otherwise
     */
    bool canUse(Player* player) const;

protected:
    std::string description_;         ///< Card description text
    CardType type_ = CardType::SKILL; ///< Card type
    CardRarity rarity_ = CardRarity::COMMON; ///< Card rarity
    CardTarget target_ = CardTarget::NONE;   ///< Card target type
    int cost_ = 1;                    ///< Energy cost to play
    bool upgradable_ = true;          ///< Whether card can be upgraded
    bool upgraded_ = false;           ///< Whether card is upgraded
    std::string classRestriction_;    ///< Class restriction (empty if none)

    // Fields for upgraded stats, loaded from JSON "upgrade_details"
    bool hasUpgradeDetails_ = false;
    std::string nameUpgraded_;
    std::string descriptionUpgraded_;
    int costUpgraded_ = -1; // -1 indicates not set or no change
    int damage_ = 0; // Base damage
    int damageUpgraded_ = -1;
    int block_ = 0; // Base block
    int blockUpgraded_ = -1;
    int magicNumber_ = 0; // Base magic number
    int magicNumberUpgraded_ = -1;
    // Add other specific stats if needed, e.g., drawUpgraded_, energyGainUpgraded_
    
    /**
     * @brief Implementation of card effect
     * @param player Player playing the card
     * @param targetIndex Index of the target (if applicable)
     * @param combat Current combat instance
     * @return True if effect succeeded, false otherwise
     */
    virtual bool onPlay(Player* player, int targetIndex, Combat* combat);
    
    /**
     * @brief Fallback implementation for basic card effects when JSON processing fails
     * @param player Player playing the card
     * @param targetIndex Index of the target (if applicable)
     * @param combat Current combat instance
     * @return True if effect succeeded, false otherwise
     */
    virtual bool fallbackCardEffect(Player* player, int targetIndex, Combat* combat);
};

} // namespace deckstiny 

#endif // DECKSTINY_CORE_CARD_H