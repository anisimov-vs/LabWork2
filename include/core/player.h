#pragma once

#include "core/character.h"
#include <vector>
#include <memory>
#include <string> // Required for std::string

namespace deckstiny {

// Forward declarations
class Card;
class Relic;
class Combat; // Forward declaration for Combat

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
     * @param maxHealth Maximum health points
     * @param baseEnergy Base energy per turn
     * @param initialHandSize Initial number of cards to draw each turn
     */
    Player(const std::string& id, const std::string& name,
           int maxHealth, int baseEnergy, int initialHandSize);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Player() = default;
    
    /**
     * @brief Get the player's class (ID) as a string, converted to uppercase.
     * @return Uppercase string representation of the player's ID.
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
     * @brief Add a card to player's deck (specifically to draw pile)
     * @param card Card to add
     */
    void addCardToDeck(std::shared_ptr<Card> card);

    /**
     * @brief Removes a specific card from the player's deck (all piles).
     * @param cardId The ID of the card to remove.
     * @param removeAllInstances If true, removes all copies. If false, removes one copy.
     * @return True if at least one card was removed, false otherwise.
     */
    bool removeCardFromDeck(const std::string& cardId, bool removeAllInstances = false);

    // Test-specific helpers
    void clearDrawPile() { drawPile_.clear(); }
    void clearDiscardPile() { discardPile_.clear(); }
    void clearHand() { hand_.clear(); }

    /**
     * @brief Set the current combat instance for the player.
     * @param combat Pointer to the current Combat object.
     */
    void setCurrentCombat(Combat* combat);

    /**
     * @brief Get the current combat instance for the player.
     * @return Pointer to the current Combat object, or nullptr if not in combat.
     */
    Combat* getCurrentCombat() const;

private:
    int gold_ = 0;                                    ///< Current gold amount
    int initialHandSize_ = 5;                         ///< Initial number of cards to draw each turn
    Combat* currentCombat_ = nullptr;                 ///< Pointer to the current combat instance
    
    std::vector<std::shared_ptr<Card>> drawPile_;     ///< Cards in draw pile
    std::vector<std::shared_ptr<Card>> discardPile_;  ///< Cards in discard pile
    std::vector<std::shared_ptr<Card>> hand_;         ///< Cards in hand
    std::vector<std::shared_ptr<Card>> exhaustPile_;  ///< Cards in exhaust pile
    
    std::vector<std::shared_ptr<Relic>> relics_;      ///< Player's relics
};

} // namespace deckstiny 