#include "core/entity.h"

namespace deckstiny {

Entity::Entity(const std::string& id, const std::string& name)
    : id_(id), name_(name) {
}

const std::string& Entity::getId() const {
    return id_;
}

const std::string& Entity::getName() const {
    return name_;
}

void Entity::setName(const std::string& name) {
    name_ = name;
}

bool Entity::loadFromJson(const nlohmann::json& json) {
    try {
        if (json.contains("id")) {
            id_ = json["id"].get<std::string>();
        }
        
        if (json.contains("name")) {
            name_ = json["name"].get<std::string>();
        }
        
        return true;
    } catch (const std::exception& e) {
        // TODO: Add proper error handling/logging
        return false;
    }
}

std::unique_ptr<Entity> Entity::clone() const {
    return std::make_unique<Entity>(id_, name_);
}

} // namespace deckstiny 