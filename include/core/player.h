#pragma once

#include "core/character.h"
#include <vector>
#include <memory>

namespace deckstiny {

// Forward declarations
class Card;
class Relic;

/**
 * @enum PlayerClass
 * @brief Represents different playable character classes
 */
enum class PlayerClass {
    IRONCLAD,
    SILENT,
    DEFECT,
    WATCHER,
    CUSTOM
};

/**
 * @class Player
 * @brief Represents the player character in the game
 * 
 * Extends Character with player-specific functionality like
 * deck management, gold, relics, etc.
 */
class Player : public Character {
public:
    /**
     * @brief Default constructor
     */
    Player() = default;
    
    /**
     * @brief Constructor with all player parameters
     * @param id Unique identifier
     * @param name Display name
     * @param playerClass Character class
     * @param maxHealth Maximum health points
     * @param baseEnergy Base energy per turn
     * @param initialHandSize Initial number of cards to draw each turn
     */
    Player(const std::string& id, const std::string& name, PlayerClass playerClass, 
           int maxHealth, int baseEnergy, int initialHandSize);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Player() = default;
    
    /**
     * @brief Get the player's class
     * @return Player class enum
     */
    PlayerClass getPlayerClass() const;
    
    /**
     * @brief Get the player's class as a string
     * @return String representation of the player's class
     */
    std::string getPlayerClassString() const;
    
    /**
     * @brief Get the player's gold
     * @return Current gold amount
     */
    int getGold() const;
    
    /**
     * @brief Add gold to the player
     * @param amount Amount of gold to add
     */
    void addGold(int amount);
    
    /**
     * @brief Spend gold
     * @param amount Amount of gold to spend
     * @return True if successful, false if insufficient gold
     */
    bool spendGold(int amount);
    
    /**
     * @brief Get the player's draw pile
     * @return Vector of card pointers in draw pile
     */
    const std::vector<std::shared_ptr<Card>>& getDrawPile() const;
    
    /**
     * @brief Get the player's discard pile
     * @return Vector of card pointers in discard pile
     */
    const std::vector<std::shared_ptr<Card>>& getDiscardPile() const;
    
    /**
     * @brief Get the player's hand
     * @return Vector of card pointers in hand
     */
    const std::vector<std::shared_ptr<Card>>& getHand() const;
    
    /**
     * @brief Get the player's exhaust pile
     * @return Vector of card pointers in exhaust pile
     */
    const std::vector<std::shared_ptr<Card>>& getExhaustPile() const;
    
    /**
     * @brief Get the player's relics
     * @return Vector of relic pointers
     */
    const std::vector<std::shared_ptr<Relic>>& getRelics() const;
    
    /**
     * @brief Add a card to the player's deck
     * @param card Shared pointer to the card to add
     * @param destination Where to add the card (draw, discard, hand, exhaust)
     */
    void addCard(std::shared_ptr<Card> card, const std::string& destination = "draw");
    
    /**
     * @brief Add a relic to the player
     * @param relic Shared pointer to the relic to add
     */
    void addRelic(std::shared_ptr<Relic> relic);
    
    /**
     * @brief Draw cards from the draw pile
     * @param count Number of cards to draw
     * @return Number of cards actually drawn
     */
    int drawCards(int count);
    
    /**
     * @brief Discard cards from hand
     * @param indices Indices of cards to discard
     * @return Number of cards actually discarded
     */
    int discardCards(const std::vector<int>& indices);
    
    /**
     * @brief Discard all cards in hand
     * @return True if successful, false otherwise
     */
    bool discardHand();
    
    /**
     * @brief Discard a single card from hand by index
     * @param index Index of the card to discard
     * @return True if successful, false otherwise
     */
    bool discardCard(int index);
    
    /**
     * @brief Exhaust a card from hand
     * @param index Index of the card to exhaust
     * @return True if successful, false otherwise
     */
    bool exhaustCard(int index);
    
    /**
     * @brief Shuffle the discard pile into the draw pile
     */
    void shuffleDiscardIntoDraw();
    
    /**
     * @brief Shuffle the draw pile
     */
    void shuffleDrawPile();
    
    /**
     * @brief Begin combat setup
     * @param shuffleDeck Whether to shuffle the deck at start
     */
    void beginCombat(bool shuffleDeck = true);
    
    /**
     * @brief Start of turn processing
     */
    void startTurn() override;
    
    /**
     * @brief End of turn processing
     */
    void endTurn() override;
    
    /**
     * @brief End combat cleanup
     */
    void endCombat();
    
    /**
     * @brief Load player data from JSON
     * @param json JSON object containing player data
     * @return True if loading was successful, false otherwise
     */
    bool loadFromJson(const nlohmann::json& json) override;
    
    /**
     * @brief Create a clone of this player
     * @return Unique pointer to a new player with the same properties
     */
    std::unique_ptr<Entity> clone() const override;

    /**
     * @brief Increase maximum health
     * @param amount Amount to increase max health by
     */
    void increaseMaxHealth(int amount);

    /**
     * @brief Add a card to player's deck
     * @param card Card to add
     */
    void addCardToDeck(std::shared_ptr<Card> card);

private:
    PlayerClass playerClass_ = PlayerClass::IRONCLAD; ///< Player's class
    int gold_ = 0;                                    ///< Current gold amount
    int initialHandSize_ = 5;                         ///< Initial number of cards to draw each turn
    
    std::vector<std::shared_ptr<Card>> drawPile_;     ///< Cards in draw pile
    std::vector<std::shared_ptr<Card>> discardPile_;  ///< Cards in discard pile
    std::vector<std::shared_ptr<Card>> hand_;         ///< Cards in hand
    std::vector<std::shared_ptr<Card>> exhaustPile_;  ///< Cards in exhaust pile
    
    std::vector<std::shared_ptr<Relic>> relics_;      ///< Player's relics
};

} // namespace deckstiny 