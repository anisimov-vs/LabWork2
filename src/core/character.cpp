// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

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

void Character::setMaxHealth(int newMaxHealthValue) {
    int oldMaxHealth = maxHealth_;
    int oldCurrentHealth = currentHealth_;
    maxHealth_ = std::max(1, newMaxHealthValue);
    currentHealth_ = std::min(currentHealth_, maxHealth_);
    LOG_INFO("character_setmaxhealth", getName() + " setMaxHealth. Old MaxHP: " + std::to_string(oldMaxHealth) + " -> New MaxHP: " + std::to_string(maxHealth_) + ". Old HP: " + std::to_string(oldCurrentHealth) + " -> New HP (after cap): " + std::to_string(currentHealth_) + ". Requested New MaxHP: " + std::to_string(newMaxHealthValue));
}

bool Character::isAlive() const {
    return currentHealth_ > 0;
}

int Character::takeDamage(int amount) {
    if (amount <= 0) {
        return 0;
    }
    
    int modifiedAmount = amount;
    if (hasStatusEffect("vulnerable")) {
        modifiedAmount = static_cast<int>(std::round(modifiedAmount * 1.5));
        LOG_DEBUG("combat", getName() + " is Vulnerable, incoming damage increased to " + std::to_string(modifiedAmount));
    }
    
    std::string entityType = (dynamic_cast<Player*>(this)) ? "Player" : "Enemy";
    LOG_DEBUG("combat", entityType + " " + getName() + " taking " + std::to_string(modifiedAmount) + " damage with " + std::to_string(block_) + " block");
    
    int remainingDamage = modifiedAmount;
    if (block_ > 0) {
        int blockUsed = std::min(block_, remainingDamage);
        block_ -= blockUsed;
        remainingDamage -= blockUsed;
        
        LOG_DEBUG("combat", entityType + " " + getName() + " blocked " + std::to_string(blockUsed) + 
                 " damage, " + std::to_string(block_) + " block remaining");
    }
    
    if (remainingDamage > 0) {
        int oldHealth = currentHealth_;
        currentHealth_ = std::max(0, currentHealth_ - remainingDamage);
        LOG_DEBUG("combat", entityType + " " + getName() + " took " + std::to_string(oldHealth - currentHealth_) + 
                 " health damage, health now " + std::to_string(currentHealth_));
    }
    
    return amount;
}

int Character::heal(int amount) {
    if (amount <= 0 || !isAlive()) {
        LOG_DEBUG("character_heal", getName() + " heal called with amount " + std::to_string(amount) + " but no healing done (amount<=0 or !isAlive).");
        return 0;
    }
    
    int oldHealth = currentHealth_;
    currentHealth_ = std::min(maxHealth_, currentHealth_ + amount);
    int healedAmount = currentHealth_ - oldHealth;
    LOG_INFO("character_heal", getName() + " healed for " + std::to_string(healedAmount) + ". Health: " + std::to_string(oldHealth) + " -> " + std::to_string(currentHealth_) + " (Max: " + std::to_string(maxHealth_) + "). Requested: " + std::to_string(amount));
    return healedAmount;
}

void Character::addBlock(int amount) {
    if (amount > 0) {
        int oldBlock = block_;
        block_ += amount;
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
    if (hasStatusEffect("poison")) {
        int poison = getStatusEffect("poison");
        takeDamage(poison);
        addStatusEffect("poison", -1);
    }
}

void Character::endTurn() {
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