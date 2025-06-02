// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#ifndef DECKSTINY_CORE_EVENT_H
#define DECKSTINY_CORE_EVENT_H

#include "core/entity.h"
#include <string>
#include <vector>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>

namespace deckstiny {

// Forward declarations
class Game;
class Player;

/**
 * @brief Represents an effect in an event choice
 */
struct EventEffect {
    std::string type;  ///< Effect type (e.g., "GAIN_GOLD", "LOSE_HEALTH", "ADD_CARD", etc.)
    int value = 0;     ///< Value of the effect (amount of gold, health, etc.)
    std::string target;///< Target for the effect (e.g., card ID for ADD_CARD)
};

/**
 * @brief Represents a choice in an event
 */
struct EventChoice {
    std::string text;                ///< Text displayed for the choice
    int goldCost = 0;                ///< Gold cost for this choice (0 if none)
    int healthCost = 0;              ///< Health cost for this choice (0 if none)
    std::vector<EventEffect> effects;///< Effects that happen when this choice is selected
    std::string resultText;          ///< Text displayed after selecting this choice
};

/**
 * @brief Represents an event encounter
 */
class Event : public Entity {
public:
    /**
     * @brief Construct a new Event object
     */
    Event() = default;
    
    /**
     * @brief Constructor with parameters
     * @param id Event unique identifier
     * @param name Display name for the event
     * @param description Initial event description
     */
    Event(const std::string& id, const std::string& name, const std::string& description);
    
    /**
     * @brief Destroy the Event object
     */
    virtual ~Event() = default;
    
    /**
     * @brief Get the event ID
     * @return Event ID
     */
    const std::string& getId() const;
    
    /**
     * @brief Get the name of the event
     * @return Event name
     */
    std::string getName() const { return name_; }
    
    /**
     * @brief Get the description of the event
     * @return Event description
     */
    std::string getDescription() const { return description_; }
    
    /**
     * @brief Get the image path (if any)
     * @return Image path
     */
    const std::string& getImagePath() const;
    
    /**
     * @brief Get all choices for this event
     * @return Vector of event choices
     */
    const std::vector<EventChoice>& getAllChoices() const { return choices_; }
    
    /**
     * @brief Get available choices based on player state
     * @param player Current player
     * @return Vector of available choices
     */
    const std::vector<EventChoice>& getAvailableChoices(Player* player) const;
    
    /**
     * @brief Process a selected choice
     * @param choiceIndex Index of the selected choice
     * @param game Game instance
     * @return Result text for the choice
     */
    std::string processChoice(int choiceIndex, Game* game);
    
    /**
     * @brief Load event data from JSON
     * @param json JSON data
     * @return True if loading succeeded, false otherwise
     */
    bool loadFromJson(const nlohmann::json& json) override;
    
    /**
     * @brief Create a clone of this event
     * @return Unique pointer to the cloned event
     */
    std::unique_ptr<Entity> clone() const override;

private:
    std::string id_;                     ///< Unique identifier
    std::string name_;                   ///< Display name
    std::string description_;            ///< Event description text
    std::string imagePath_;              ///< Path to event image (if any)
    std::vector<EventChoice> choices_;   ///< Available choices for this event
    
    // Cached available choices based on player state
    mutable std::vector<EventChoice> availableChoices_;
};

} // namespace deckstiny 

#endif // DECKSTINY_CORE_EVENT_H