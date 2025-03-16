#include "core/card.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/combat.h"
#include "core/game.h"
#include "util/logger.h"

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>

namespace deckstiny {

Card::Card(const std::string& id, const std::string& name, const std::string& description,
           CardType type, CardRarity rarity, CardTarget target, int cost, bool upgradable)
    : Entity(id, name), description_(description), type_(type), rarity_(rarity), 
      target_(target), cost_(cost), upgradable_(upgradable), upgraded_(false) {
}

const std::string& Card::getDescription() const {
    return description_;
}

void Card::setDescription(const std::string& description) {
    description_ = description;
}

CardType Card::getType() const {
    return type_;
}

CardRarity Card::getRarity() const {
    return rarity_;
}

CardTarget Card::getTarget() const {
    return target_;
}

int Card::getCost() const {
    return cost_;
}

void Card::setCost(int cost) {
    cost_ = std::max(0, cost);
}

bool Card::isUpgradable() const {
    return upgradable_;
}

bool Card::isUpgraded() const {
    return upgraded_;
}

bool Card::upgrade() {
    if (!upgradable_ || upgraded_) {
        return false;
    }
    
    upgraded_ = true;
    
    // Default upgrade behavior - override in subclasses for specific effects
    // For now, just reduce cost by 1 if possible
    if (cost_ > 0) {
        cost_--;
    }
    
    return true;
}

bool Card::canPlay(Player* player, int targetIndex, Combat* combat) const {
    if (!player || !combat) {
        return false;
    }
    
    // Check if player has enough energy
    if (player->getEnergy() < cost_) {
        return false;
    }
    
    // Check if target is valid
    switch (target_) {
        case CardTarget::NONE:
            // No target needed
            return true;
            
        case CardTarget::SELF:
            // Target should be -1 or 0 (self)
            return targetIndex < 0 || targetIndex == 0;
            
        case CardTarget::SINGLE_ENEMY:
            // Target should be a valid enemy index
            return targetIndex >= 0 && targetIndex < static_cast<int>(combat->getEnemyCount()) &&
                   combat->getEnemy(targetIndex) && combat->getEnemy(targetIndex)->isAlive();
            
        case CardTarget::ALL_ENEMIES:
            // No specific target needed
            return true;
            
        case CardTarget::SINGLE_ALLY:
            // Not implemented yet
            return false;
            
        case CardTarget::ALL_ALLIES:
            // Not implemented yet
            return false;
            
        default:
            return false;
    }
}

bool Card::play(Player* player, int targetIndex, Combat* combat) {
    if (!canPlay(player, targetIndex, combat)) {
        return false;
    }
    
    // Use energy
    if (!player->useEnergy(cost_)) {
        return false;
    }
    
    // Find this card's index in the player's hand
    const auto& hand = player->getHand();
    int cardIndex = -1;
    
    for (size_t i = 0; i < hand.size(); ++i) {
        if (hand[i].get() == this) {
            cardIndex = i;
            break;
        }
    }
    
    if (cardIndex == -1) {
        LOG_ERROR("card", "Cannot find card in player's hand");
        return false;
    }
    
    // Execute card effect
    bool success = onPlay(player, targetIndex, combat);
    
    if (success) {
        // If it's not a POWER card, move it to discard pile
        if (type_ != CardType::POWER) {
            std::vector<int> indices = {cardIndex};
            player->discardCards(indices);
            LOG_INFO("card", "Card " + getName() + " moved to discard pile");
        } else {
            LOG_INFO("card", "POWER card " + getName() + " exhausted after play");
            player->exhaustCard(cardIndex);
        }
    }
    
    return success;
}

bool Card::loadFromJson(const nlohmann::json& json) {
    if (!Entity::loadFromJson(json)) {
        return false;
    }
    
    try {
        if (json.contains("description")) {
            description_ = json["description"].get<std::string>();
        }
        
        if (json.contains("type")) {
            std::string typeStr = json["type"].get<std::string>();
            if (typeStr == "ATTACK") {
                type_ = CardType::ATTACK;
            } else if (typeStr == "SKILL") {
                type_ = CardType::SKILL;
            } else if (typeStr == "POWER") {
                type_ = CardType::POWER;
            } else if (typeStr == "STATUS") {
                type_ = CardType::STATUS;
            } else if (typeStr == "CURSE") {
                type_ = CardType::CURSE;
            }
        }
        
        if (json.contains("rarity")) {
            std::string rarityStr = json["rarity"].get<std::string>();
            if (rarityStr == "COMMON") {
                rarity_ = CardRarity::COMMON;
            } else if (rarityStr == "UNCOMMON") {
                rarity_ = CardRarity::UNCOMMON;
            } else if (rarityStr == "RARE") {
                rarity_ = CardRarity::RARE;
            } else if (rarityStr == "SPECIAL") {
                rarity_ = CardRarity::SPECIAL;
            } else if (rarityStr == "BASIC") {
                rarity_ = CardRarity::BASIC;
            }
        }
        
        if (json.contains("target")) {
            std::string targetStr = json["target"].get<std::string>();
            if (targetStr == "NONE") {
                target_ = CardTarget::NONE;
            } else if (targetStr == "SELF") {
                target_ = CardTarget::SELF;
            } else if (targetStr == "SINGLE_ENEMY") {
                target_ = CardTarget::SINGLE_ENEMY;
            } else if (targetStr == "ALL_ENEMIES") {
                target_ = CardTarget::ALL_ENEMIES;
            } else if (targetStr == "SINGLE_ALLY") {
                target_ = CardTarget::SINGLE_ALLY;
            } else if (targetStr == "ALL_ALLIES") {
                target_ = CardTarget::ALL_ALLIES;
            }
        }
        
        if (json.contains("cost")) {
            cost_ = json["cost"].get<int>();
        }
        
        if (json.contains("upgradable")) {
            upgradable_ = json["upgradable"].get<bool>();
        }
        
        if (json.contains("upgraded")) {
            upgraded_ = json["upgraded"].get<bool>();
        }
        
        // Load class restriction if present
        if (json.contains("class")) {
            classRestriction_ = json["class"].get<std::string>();
            // Convert to uppercase for consistency
            std::transform(classRestriction_.begin(), classRestriction_.end(), 
                         classRestriction_.begin(), ::toupper);
            // Special case: "ALL" means available to all classes
            if (classRestriction_ == "ALL") {
                LOG_INFO("card", "Card " + getName() + " is available to all classes");
            } else if (!classRestriction_.empty()) {
                LOG_INFO("card", "Card " + getName() + " is restricted to class: " + classRestriction_);
            }
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading card from JSON: " << e.what() << std::endl;
        return false;
    }
}

std::unique_ptr<Entity> Card::clone() const {
    return std::make_unique<Card>(*this);
}

std::shared_ptr<Card> Card::cloneCard() const {
    return std::make_shared<Card>(*this);
}

bool Card::onPlay(Player* player, int targetIndex, Combat* combat) {
    // We need to process the card effects from the card's JSON data
    // First, let's try to find the card in the file
    try {
        std::ifstream file("data/cards/" + getId() + ".json");
        if (!file.is_open()) {
            // Fall back to hardcoded effects if JSON not found
            return fallbackCardEffect(player, targetIndex, combat);
        }
        
        nlohmann::json cardData;
        file >> cardData;
        
        if (!cardData.contains("effects") || !cardData["effects"].is_array()) {
            // No effects found in JSON, use fallback
            return fallbackCardEffect(player, targetIndex, combat);
        }
        
        bool success = false;
        
        // Process each effect in the order defined
        for (const auto& effect : cardData["effects"]) {
            std::string effectType = effect.value("type", "");
            std::string effectTarget = effect.value("target", "");
            int effectValue = effect.value("value", 0);
            
            // Apply upgraded value if card is upgraded
            if (upgraded_ && effect.contains("upgraded_value")) {
                effectValue = effect.value("upgraded_value", effectValue);
            }
            
            if (effectType == "damage") {
                // Apply damage effect
                if (effectTarget == "enemy" && target_ == CardTarget::SINGLE_ENEMY) {
                    Enemy* enemy = combat->getEnemy(targetIndex);
                    if (enemy) {
                        enemy->takeDamage(effectValue);
                        success = true;
                        
                        // Check if enemy was defeated and handle death
                        if (!enemy->isAlive()) {
                            // Find the index of this enemy in the combat
                            for (size_t idx = 0; idx < combat->getEnemyCount(); ++idx) {
                                if (combat->getEnemy(idx) == enemy) {
                                    combat->handleEnemyDeath(idx);
                                    break;
                                }
                            }
                            
                            // Check if all enemies are defeated
                            if (combat->areAllEnemiesDefeated()) {
                                // Mark combat as over
                                if (!combat->isCombatOver()) {
                                    combat->end(true);
                                    
                                    // If the game instance is available, call endCombat directly
                                    // but only if we haven't already transitioned
                                    if (auto game = combat->getGame()) {
                                        LOG_INFO("card", "All enemies defeated, game pointer available, calling endCombat");
                                        game->endCombat(true);
                                    } else {
                                        LOG_WARNING("card", "All enemies defeated but game pointer not available!");
                                    }
                                } else {
                                    LOG_INFO("card", "Combat already marked as over, not calling endCombat again");
                                }
                            }
                        }
                    }
                } else if (effectTarget == "all_enemies" || target_ == CardTarget::ALL_ENEMIES) {
                    bool anyEnemyDefeated = false;
                    
                    for (size_t i = 0; i < combat->getEnemyCount(); ++i) {
                        Enemy* enemy = combat->getEnemy(i);
                        if (enemy && enemy->isAlive()) {
                            enemy->takeDamage(effectValue);
                            
                            // Check if this enemy was just defeated
                            if (!enemy->isAlive()) {
                                combat->handleEnemyDeath(i);
                                anyEnemyDefeated = true;
                            }
                        }
                    }
                    
                    // If any enemy was defeated, check if all are defeated now
                    if (anyEnemyDefeated && combat->areAllEnemiesDefeated()) {
                        // Mark combat as over
                        if (!combat->isCombatOver()) {
                            combat->end(true);
                            
                            // If the game instance is available, call endCombat directly
                            // but only if we haven't already transitioned
                            if (auto game = combat->getGame()) {
                                LOG_INFO("card", "All enemies defeated, game pointer available, calling endCombat");
                                game->endCombat(true);
                            } else {
                                LOG_WARNING("card", "All enemies defeated but game pointer not available!");
                            }
                        } else {
                            LOG_INFO("card", "Combat already marked as over, not calling endCombat again");
                        }
                    }
                    
                    success = true;
                }
            } else if (effectType == "block") {
                // Apply block effect
                if (effectTarget == "self" || effectTarget == "player") {
                    player->addBlock(effectValue);
                    success = true;
                }
            } else if (effectType == "status_effect") {
                // Apply status effect
                std::string statusEffect = effect.value("effect", "");
                if (statusEffect.empty()) continue;
                
                if (effectTarget == "enemy" && target_ == CardTarget::SINGLE_ENEMY) {
                    Enemy* enemy = combat->getEnemy(targetIndex);
                    if (enemy) {
                        enemy->addStatusEffect(statusEffect, effectValue);
                        success = true;
                    }
                } else if (effectTarget == "all_enemies" || target_ == CardTarget::ALL_ENEMIES) {
                    for (size_t i = 0; i < combat->getEnemyCount(); ++i) {
                        Enemy* enemy = combat->getEnemy(i);
                        if (enemy && enemy->isAlive()) {
                            enemy->addStatusEffect(statusEffect, effectValue);
                        }
                    }
                    success = true;
                } else if (effectTarget == "self" || effectTarget == "player") {
                    player->addStatusEffect(statusEffect, effectValue);
                    success = true;
                }
            }
            // Add more effect types as needed
        }
        
        return success;
    } catch (const std::exception& e) {
        std::cerr << "Error processing card effects for " << getId() << ": " << e.what() << std::endl;
        // Fall back to hardcoded effects if JSON processing fails
        return fallbackCardEffect(player, targetIndex, combat);
    }
}

// Fallback method for basic card effects
bool Card::fallbackCardEffect(Player* player, int targetIndex, Combat* combat) {
    // For now, just handle basic attack and block effects
    if (type_ == CardType::ATTACK) {
        // Deal damage to target
        if (target_ == CardTarget::SINGLE_ENEMY) {
            Enemy* enemy = combat->getEnemy(targetIndex);
            if (enemy) {
                // For now, just deal 6 damage (or 9 if upgraded)
                int damage = upgraded_ ? 9 : 6;
                enemy->takeDamage(damage);
                return true;
            }
        } else if (target_ == CardTarget::ALL_ENEMIES) {
            // Deal damage to all enemies
            int damage = upgraded_ ? 9 : 6;
            for (size_t i = 0; i < combat->getEnemyCount(); ++i) {
                Enemy* enemy = combat->getEnemy(i);
                if (enemy && enemy->isAlive()) {
                    enemy->takeDamage(damage);
                }
            }
            return true;
        }
    } else if (type_ == CardType::SKILL) {
        // Apply block or other effects
        if (target_ == CardTarget::SELF) {
            // For now, just add 5 block (or 8 if upgraded)
            int block = upgraded_ ? 8 : 5;
            player->addBlock(block);
            return true;
        }
    }
    
    return false;
}

bool Card::needsTarget() const {
    // Check if the card targets a single enemy
    return target_ == CardTarget::SINGLE_ENEMY;
}

const std::string& Card::getClassRestriction() const {
    return classRestriction_;
}

void Card::setClassRestriction(const std::string& className) {
    classRestriction_ = className;
}

bool Card::canUse(Player* player) const {
    if (!player) return false;
    if (classRestriction_.empty()) return true;
    if (classRestriction_ == "ALL") return true;
    return player->getPlayerClassString() == classRestriction_;
}

} // namespace deckstiny 