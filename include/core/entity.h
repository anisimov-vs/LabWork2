#pragma once

#include <string>
#include <memory>
#include <nlohmann/json.hpp>

namespace deckstiny {

/**
 * @class Entity
 * @brief Base class for all game entities
 * 
 * This class serves as the foundation for all entities in the game,
 * providing common properties like ID and name.
 */
class Entity {
public:
    /**
     * @brief Default constructor
     */
    Entity() = default;
    
    /**
     * @brief Constructor with id and name
     * @param id Unique identifier for the entity
     * @param name Display name of the entity
     */
    Entity(const std::string& id, const std::string& name);
    
    /**
     * @brief Virtual destructor
     */
    virtual ~Entity() = default;
    
    /**
     * @brief Get the entity's id
     * @return String containing the entity's id
     */
    const std::string& getId() const;
    
    /**
     * @brief Get the entity's name
     * @return String containing the entity's name
     */
    const std::string& getName() const;
    
    /**
     * @brief Set the entity's name
     * @param name New name for the entity
     */
    void setName(const std::string& name);
    
    /**
     * @brief Load entity data from JSON
     * @param json JSON object containing entity data
     * @return True if loading was successful, false otherwise
     */
    virtual bool loadFromJson(const nlohmann::json& json);
    
    /**
     * @brief Create a clone of this entity
     * @return Unique pointer to a new entity with the same properties
     */
    virtual std::unique_ptr<Entity> clone() const;

private:
    std::string id_;   ///< Unique identifier
    std::string name_; ///< Display name
};

} // namespace deckstiny 