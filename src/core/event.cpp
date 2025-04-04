#include "core/event.h"
#include "core/player.h"
#include "core/game.h"
#include "util/logger.h"
#include <iostream>
#include <fstream>

namespace deckstiny {

// Constructor implementation
Event::Event(const std::string& id, const std::string& name, const std::string& description)
    : id_(id), name_(name), description_(description) {
    LOG_INFO("event", "Created event: " + name);
}

// getId implementation
const std::string& Event::getId() const {
    return id_;
}

// getImagePath implementation
const std::string& Event::getImagePath() const {
    return imagePath_;
}

const std::vector<EventChoice>& Event::getAvailableChoices(Player* player) const {
    if (!player) {
        LOG_ERROR("event", "Invalid player");
        return choices_; // Return all choices if no player
    }
    
    // Clear previous available choices
    availableChoices_.clear();
    
    // Check each choice against player state
    for (const auto& choice : choices_) {
        bool available = true;
        
        // Check gold requirements
        if (choice.goldCost > 0 && player->getGold() < choice.goldCost) {
            available = false;
        }
        
        // Check health requirements
        if (choice.healthCost > 0 && player->getHealth() < choice.healthCost) {
            available = false;
        }
        
        // If all requirements are met, add to available choices
        if (available) {
            availableChoices_.push_back(choice);
        }
    }
    
    return availableChoices_;
}

std::string Event::processChoice(int choiceIndex, Game* game) {
    if (!game) {
        LOG_ERROR("event", "Invalid game");
        return "Error: Cannot process choice without game instance.";
    }
    
    // Check if choice is valid
    if (choiceIndex < 0 || choiceIndex >= static_cast<int>(choices_.size())) {
        LOG_ERROR("event", "Invalid choice index: " + std::to_string(choiceIndex));
        return "Invalid choice.";
    }
    
    const EventChoice& choice = choices_[choiceIndex];
    Player* player = game->getPlayer();
    
    if (!player) {
        LOG_ERROR("event", "Invalid player");
        return "Error: Cannot process choice without player.";
    }
    
    // Build detailed result text
    std::string resultText = choice.resultText + "\n\n";
    
    // Apply effects and build detailed result text
    for (const auto& effect : choice.effects) {
        // Process different effect types
        if (effect.type == "GAIN_GOLD") {
            player->addGold(effect.value);
            resultText += "You gained " + std::to_string(effect.value) + " gold.\n";
            LOG_INFO("event", "Player gained " + std::to_string(effect.value) + " gold");
        } else if (effect.type == "LOSE_GOLD") {
            player->addGold(-effect.value);
            resultText += "You lost " + std::to_string(effect.value) + " gold.\n";
            LOG_INFO("event", "Player lost " + std::to_string(effect.value) + " gold");
        } else if (effect.type == "GAIN_HEALTH") {
            int oldHealth = player->getHealth();
            player->heal(effect.value);
            int healed = player->getHealth() - oldHealth;
            resultText += "You healed " + std::to_string(healed) + " HP.\n";
            LOG_INFO("event", "Player gained " + std::to_string(effect.value) + " health");
        } else if (effect.type == "LOSE_HEALTH") {
            player->takeDamage(effect.value);
            resultText += "You took " + std::to_string(effect.value) + " damage.\n";
            LOG_INFO("event", "Player lost " + std::to_string(effect.value) + " health");
        } else if (effect.type == "INCREASE_MAX_HEALTH") {
            player->increaseMaxHealth(effect.value);
            resultText += "Your max health increased by " + std::to_string(effect.value) + ".\n";
            LOG_INFO("event", "Player's max health increased by " + std::to_string(effect.value));
        } else if (effect.type == "ADD_CARD") {
            // Add card to player's deck
            game->addCardToDeck(effect.target);
            resultText += "You added " + effect.target + " to your deck.\n";
            LOG_INFO("event", "Added card to player's deck: " + effect.target);
        } else if (effect.type == "REMOVE_CARD") {
            // Remove card from player's deck (not yet implemented)
            resultText += "You removed a card from your deck.\n";
            LOG_INFO("event", "Not implemented: Remove card from deck");
        } else if (effect.type == "ADD_RELIC") {
            // Add relic to player
            game->addRelic(effect.target);
            resultText += "You gained the " + effect.target + " relic.\n";
            LOG_INFO("event", "Added relic to player: " + effect.target);
        } else if (effect.type == "RANDOM_RELIC") {
            // Add random relic (not fully implemented)
            resultText += "You gained a random relic.\n";
            LOG_INFO("event", "Not fully implemented: Add random relic");
        } else if (effect.type == "UPGRADE_CARD") {
            // Upgrade a card
            std::string upgradedCard = game->upgradeCard();
            resultText += "You upgraded " + upgradedCard + ".\n";
            LOG_INFO("event", "Player upgraded a card: " + upgradedCard);
        } else {
            LOG_WARNING("event", "Unknown effect type: " + effect.type);
        }
    }
    
    return resultText;
}

bool Event::loadFromJson(const nlohmann::json& json) {
    try {
        // Load basic event info
        id_ = json["id"];
        name_ = json["name"];
        description_ = json["description"];
        
        // Load choices
        choices_.clear();
        
        if (json.contains("choices") && json["choices"].is_array()) {
            for (const auto& choiceJson : json["choices"]) {
                EventChoice choice;
                choice.text = choiceJson["text"];
                
                // Load result text
                if (choiceJson.contains("resultText")) {
                    choice.resultText = choiceJson["resultText"];
                }
                
                // Load requirements
                if (choiceJson.contains("requiresGold")) {
                    choice.goldCost = choiceJson["requiresGold"];
                }
                
                if (choiceJson.contains("requiresHealth")) {
                    choice.healthCost = choiceJson["requiresHealth"];
                }
                
                // Load effects
                if (choiceJson.contains("effects") && choiceJson["effects"].is_array()) {
                    for (const auto& effectJson : choiceJson["effects"]) {
                        EventEffect effect;
                        effect.type = effectJson["effect"];
                        
                        if (effectJson.contains("value")) {
                            if (effectJson["value"].is_number()) {
                                effect.value = effectJson["value"];
                            } else if (effectJson["value"].is_string()) {
                                effect.target = effectJson["value"];
                            }
                        }
                        
                        choice.effects.push_back(effect);
                    }
                }
                
                choices_.push_back(choice);
            }
        }
        
        LOG_INFO("event", "Loaded event: " + name_ + " with " + std::to_string(choices_.size()) + " choices");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("event", "Error loading event from JSON: " + std::string(e.what()));
        return false;
    }
}

std::unique_ptr<Entity> Event::clone() const {
    auto clonedEvent = std::make_unique<Event>();
    clonedEvent->id_ = id_;
    clonedEvent->name_ = name_;
    clonedEvent->description_ = description_;
    clonedEvent->choices_ = choices_;
    return clonedEvent;
}

} // namespace deckstiny 