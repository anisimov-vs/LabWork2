#include "core/character.h"
#include "core/player.h"
#include "util/logger.h"
#include <algorithm>
#include <string>

namespace deckstiny {

Character::Character(const std::string& id, const std::string& name, int maxHealth, int baseEnergy)
    : Entity(id, name), maxHealth_(maxHealth), currentHealth_(maxHealth), baseEnergy_(baseEnergy), currentEnergy_(0) {
}

int Character::getHealth() const {
    return currentHealth_;
}

void Character::setHealth(int health) {
    currentHealth_ = std::min(std::max(0, health), maxHealth_);
}

int Character::getMaxHealth() const {
    return maxHealth_;
}

void Character::setMaxHealth(int maxHealth) {
    maxHealth_ = std::max(1, maxHealth);
    currentHealth_ = std::min(currentHealth_, maxHealth_);
}

bool Character::isAlive() const {
    return currentHealth_ > 0;
}

int Character::takeDamage(int amount) {
    if (amount <= 0) {
        return 0;
    }
    
    // Add debug logging
    std::string entityType = (dynamic_cast<Player*>(this)) ? "Player" : "Enemy";
    LOG_DEBUG("combat", entityType + " " + getName() + " taking " + std::to_string(amount) + " damage with " + std::to_string(block_) + " block");
    
    // Apply block
    int remainingDamage = amount;
    if (block_ > 0) {
        int blockUsed = std::min(block_, remainingDamage);
        block_ -= blockUsed;
        remainingDamage -= blockUsed;
        
        // More detailed block usage logging
        LOG_DEBUG("combat", entityType + " " + getName() + " blocked " + std::to_string(blockUsed) + 
                 " damage, " + std::to_string(block_) + " block remaining");
    }
    
    // Apply remaining damage to health
    if (remainingDamage > 0) {
        int oldHealth = currentHealth_;
        currentHealth_ = std::max(0, currentHealth_ - remainingDamage);
        LOG_DEBUG("combat", entityType + " " + getName() + " took " + std::to_string(oldHealth - currentHealth_) + 
                 " health damage, health now " + std::to_string(currentHealth_));
    }
    
    // Return the original damage amount, as the tests expect this
    return amount;
}

int Character::heal(int amount) {
    if (amount <= 0 || !isAlive()) {
        return 0;
    }
    
    int oldHealth = currentHealth_;
    currentHealth_ = std::min(maxHealth_, currentHealth_ + amount);
    return currentHealth_ - oldHealth;
}

void Character::addBlock(int amount) {
    if (amount > 0) {
        int oldBlock = block_;
        block_ += amount;
        // Add debug logging
        std::string entityType = (dynamic_cast<Player*>(this)) ? "Player" : "Enemy";
        LOG_DEBUG("combat", entityType + " " + getName() + " block increased from " + 
                 std::to_string(oldBlock) + " to " + std::to_string(block_));
    }
}

int Character::getBlock() const {
    return block_;
}

int Character::getBaseEnergy() const {
    return baseEnergy_;
}

int Character::getEnergy() const {
    return currentEnergy_;
}

void Character::setEnergy(int energy) {
    currentEnergy_ = std::max(0, energy);
}

bool Character::useEnergy(int amount) {
    if (amount <= 0) {
        return true;
    }
    
    if (currentEnergy_ >= amount) {
        currentEnergy_ -= amount;
        return true;
    }
    
    return false;
}

void Character::resetEnergy() {
    currentEnergy_ = baseEnergy_;
}

void Character::resetBlock() {
    // Add debug logging
    if (block_ > 0) {
        std::string entityType = (dynamic_cast<Player*>(this)) ? "Player" : "Enemy";
        LOG_DEBUG("combat", entityType + " " + getName() + " block reset from " + 
                 std::to_string(block_) + " to 0");
    }
    block_ = 0;
}

void Character::addStatusEffect(const std::string& effect, int stacks) {
    if (stacks == 0) {
        return;
    }
    
    auto it = statusEffects_.find(effect);
    if (it == statusEffects_.end()) {
        if (stacks > 0) {
            statusEffects_[effect] = stacks;
        }
    } else {
        it->second += stacks;
        if (it->second <= 0) {
            statusEffects_.erase(it);
        }
    }
}

int Character::getStatusEffect(const std::string& effect) const {
    auto it = statusEffects_.find(effect);
    return (it != statusEffects_.end()) ? it->second : 0;
}

bool Character::hasStatusEffect(const std::string& effect) const {
    return statusEffects_.find(effect) != statusEffects_.end();
}

const std::unordered_map<std::string, int>& Character::getStatusEffects() const {
    return statusEffects_;
}

void Character::startTurn() {
    // Process status effects at start of turn
    // For example, poison damage
    if (hasStatusEffect("poison")) {
        int poison = getStatusEffect("poison");
        takeDamage(poison);
        addStatusEffect("poison", -1); // Reduce poison by 1
    }
}

void Character::endTurn() {
    // Process status effects at end of turn
    // For example, reduce strength from temporary buffs
    if (hasStatusEffect("temporary_strength")) {
        int tempStr = getStatusEffect("temporary_strength");
        addStatusEffect("strength", -tempStr);
        addStatusEffect("temporary_strength", -tempStr);
    }
}

bool Character::loadFromJson(const nlohmann::json& json) {
    if (!Entity::loadFromJson(json)) {
        return false;
    }
    
    try {
        if (json.contains("max_health")) {
            maxHealth_ = json["max_health"].get<int>();
            currentHealth_ = maxHealth_;
        }
        
        if (json.contains("current_health")) {
            currentHealth_ = std::min(maxHealth_, json["current_health"].get<int>());
        }
        
        if (json.contains("base_energy")) {
            baseEnergy_ = json["base_energy"].get<int>();
            currentEnergy_ = baseEnergy_;
        }
        
        if (json.contains("status_effects") && json["status_effects"].is_object()) {
            for (auto& [effect, stacks] : json["status_effects"].items()) {
                statusEffects_[effect] = stacks.get<int>();
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        // TODO: Add proper error handling/logging
        return false;
    }
}

std::unique_ptr<Entity> Character::clone() const {
    auto character = std::make_unique<Character>(getId(), getName(), maxHealth_, baseEnergy_);
    character->currentHealth_ = currentHealth_;
    character->block_ = block_;
    character->currentEnergy_ = currentEnergy_;
    character->statusEffects_ = statusEffects_;
    return character;
}

} // namespace deckstiny 