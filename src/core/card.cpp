// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include "core/card.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/combat.h"
#include "core/game.h"
#include "util/logger.h"
#include "util/path_util.h"

#include <fstream>
#include <nlohmann/json.hpp>
#include <sys/stat.h>

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
    
    if (hasUpgradeDetails_) {
        if (!nameUpgraded_.empty() && nameUpgraded_ != getName()) {
            setName(nameUpgraded_);
        }
        if (!descriptionUpgraded_.empty()) {
            description_ = descriptionUpgraded_;
        }
        if (costUpgraded_ != -1 && costUpgraded_ != cost_) {
            cost_ = costUpgraded_;
        }
        if (damageUpgraded_ != -1) {
            damage_ = damageUpgraded_;
        }
        if (blockUpgraded_ != -1) {
            block_ = blockUpgraded_;
        }
        if (magicNumberUpgraded_ != -1) {
            magicNumber_ = magicNumberUpgraded_;
        }
    } else {
    if (cost_ > 0) {
        cost_--;
        }
        if (getName().rfind('+') == std::string::npos) {
             setName(getName() + "+");
        }
    }
    
    return true;
}

bool Card::canPlay(Player* player, int targetIndex, Combat* combat) const {
    if (!player || !combat) {
        LOG_DEBUG("card_canPlay", "Called with null player or combat. Card: " + getName());
        return false;
    }
    
    int currentEnergy = player->getEnergy();
    LOG_DEBUG("card_canPlay", "Card: " + getName() + ", Player Energy: " + std::to_string(currentEnergy) + ", Card Cost: " + std::to_string(cost_));
    
    if (currentEnergy < cost_) {
        LOG_DEBUG("card_canPlay", "Energy check FAILED for " + getName() + ". Player Energy: " + std::to_string(currentEnergy) + " < Card Cost: " + std::to_string(cost_));
        return false;
    }
    LOG_DEBUG("card_canPlay", "Energy check PASSED for " + getName() + ". Player Energy: " + std::to_string(currentEnergy) + " >= Card Cost: " + std::to_string(cost_));
    
    switch (target_) {
        case CardTarget::NONE:
            LOG_DEBUG("card_canPlay", "Target check PASSED for " + getName() + " (NONE target). Returning true.");
            return true;
            
        case CardTarget::SELF:
            LOG_DEBUG("card_canPlay", "Target check PASSED for " + getName() + " (SELF target, index: " + std::to_string(targetIndex) + "). Returning true.");
            return true;
            
        case CardTarget::SINGLE_ENEMY: {
            bool targetValid = targetIndex >= 0 && targetIndex < static_cast<int>(combat->getEnemyCount()) &&
                   combat->getEnemy(targetIndex) && combat->getEnemy(targetIndex)->isAlive();
            LOG_DEBUG("card_canPlay", "Target check (SINGLE_ENEMY) for " + getName() + ": targetIndex=" + std::to_string(targetIndex) +
                                     ", enemyCount=" + std::to_string(combat->getEnemyCount()) +
                                     ", getEnemy(idx) valid=" + (combat->getEnemy(targetIndex) ? "true" : "false") +
                                     ", isAlive=" + (combat->getEnemy(targetIndex) && combat->getEnemy(targetIndex)->isAlive() ? "true" : "false") +
                                     ". Result: " + (targetValid ? "PASSED" : "FAILED"));
            return targetValid;
        }
            
        case CardTarget::ALL_ENEMIES:
            LOG_DEBUG("card_canPlay", "Target check PASSED for " + getName() + " (ALL_ENEMIES target). Returning true.");
            return true;
            
        case CardTarget::SINGLE_ALLY:
            LOG_DEBUG("card_canPlay", "Target check FAILED for " + getName() + " (SINGLE_ALLY target - Not Implemented). Returning false.");
            return false;
            
        case CardTarget::ALL_ALLIES:
            LOG_DEBUG("card_canPlay", "Target check FAILED for " + getName() + " (ALL_ALLIES target - Not Implemented). Returning false.");
            return false;
            
        default:
            LOG_DEBUG("card_canPlay", "Target check FAILED for " + getName() + " (UNKNOWN target type). Returning false.");
            return false;
    }
}

bool Card::play(Player* player, int targetIndex, Combat* combat) {
    LOG_DEBUG("card_play", "Attempting to play card: " + getName());
    if (!canPlay(player, targetIndex, combat)) {
        LOG_ERROR("card_play", "Pre-check canPlay() failed for " + getName() + ". Aborting play.");
        return false;
    }
    
    LOG_DEBUG("card_play", getName() + " canPlay() passed. Player energy before useEnergy: " + std::to_string(player->getEnergy()) + ", Card cost: " + std::to_string(cost_));

    if (!player->useEnergy(cost_)) {
        LOG_ERROR("card_play", getName() + " player->useEnergy(" + std::to_string(cost_) + ") FAILED. Player energy was: " + std::to_string(player->getEnergy()) + " (This should not happen if canPlay passed for positive cost cards).");
        return false;
    }
    LOG_DEBUG("card_play", getName() + " player->useEnergy(" + std::to_string(cost_) + ") SUCCEEDED. Player energy after useEnergy: " + std::to_string(player->getEnergy()));
    
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
    
    LOG_DEBUG("card_play", "Calling onPlay for " + getName());
    bool successOnPlay = onPlay(player, targetIndex, combat);
    LOG_DEBUG("card_play", "onPlay for " + getName() + " returned: " + (successOnPlay ? "true" : "false"));
    
    if (successOnPlay) {
        if (type_ != CardType::POWER) {
            std::vector<int> indices = {cardIndex};
            player->discardCards(indices);
            LOG_INFO("card", "Card " + getName() + " moved to discard pile");
        } else {
            LOG_INFO("card", "POWER card " + getName() + " exhausted after play");
            player->exhaustCard(cardIndex);
        }
    }
    
    LOG_DEBUG("card_play", getName() + " play() finished. Overall success: " + (successOnPlay ? "true" : "false"));
    return successOnPlay;
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
        
        if (json.contains("damage")) {
            damage_ = json["damage"].get<int>();
        }
        if (json.contains("block")) {
            block_ = json["block"].get<int>();
        }
        if (json.contains("magic_number")) {
            magicNumber_ = json["magic_number"].get<int>();
        }

        if (json.contains("class")) {
            classRestriction_ = json["class"].get<std::string>();
            std::transform(classRestriction_.begin(), classRestriction_.end(), 
                         classRestriction_.begin(), ::toupper);
            // Special case: "ALL" means available to all classes
            if (classRestriction_ == "ALL") {
                LOG_INFO("card", "Card " + getName() + " is available to all classes");
            } else if (!classRestriction_.empty()) {
                LOG_INFO("card", "Card " + getName() + " is restricted to class: " + classRestriction_);
            }
        }
        
        if (json.contains("upgrade_details") && json["upgrade_details"].is_object()) {
            const auto& upgradeJson = json["upgrade_details"];
            hasUpgradeDetails_ = true;
            nameUpgraded_ = upgradeJson.value("name", getName() + "+"); 
            descriptionUpgraded_ = upgradeJson.value("description", description_);
            costUpgraded_ = upgradeJson.value("cost", cost_);
            damageUpgraded_ = upgradeJson.value("damage", damage_); 
            blockUpgraded_ = upgradeJson.value("block", block_);
            magicNumberUpgraded_ = upgradeJson.value("magic_number", magicNumber_);
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("card", "Error loading card from JSON: " + std::string(e.what()));
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
    std::string prefix = get_data_path_prefix();

    try {
        std::string jsonFilePath = prefix + "data/cards/" + getId() + ".json";
        LOG_DEBUG("card_onPlay", "Attempting to open card JSON with get_data_path_prefix: " + jsonFilePath);

        std::ifstream file(jsonFilePath);
        if (!file.is_open()) {
            LOG_ERROR("card_onPlay", "Failed to open card JSON: " + jsonFilePath);
            return fallbackCardEffect(player, targetIndex, combat);
        }
        
        nlohmann::json cardData;
        file >> cardData;
        
        if (!cardData.contains("effects") || !cardData["effects"].is_array()) {
            LOG_WARNING("card_onPlay", "No 'effects' array in JSON for card: " + getId());
            return fallbackCardEffect(player, targetIndex, combat);
        }
        
        bool overallSuccess = true;
        
        for (const auto& effectJson : cardData["effects"]) {
            std::string effectType = effectJson.value("type", "");
            std::string effectTargetType = effectJson.value("target", "");

            int currentValue = 0;
            if (effectType == "damage" || effectType == "block") {
                currentValue = effectJson.value("value", 0);
                if (this->upgraded_ && effectJson.contains("upgraded_value")) {
                    currentValue = effectJson.value("upgraded_value", currentValue);
                }
            } else if (effectType == "apply_vulnerable" || effectType == "apply_weak" || effectType == "gain_strength") {
                currentValue = effectJson.value("value", 0);
                if (this->upgraded_) {
                    if (effectJson.contains("upgraded_value")) {
                        currentValue = effectJson.value("upgraded_value", currentValue);
                    } else if (magicNumberUpgraded_ != -1) {
                        currentValue = magicNumberUpgraded_;
                    } 
                } else if (effectJson.contains("value")){
                     currentValue = effectJson.value("value", 0);
                } else {
                     currentValue = magicNumber_;
                }
            } else if (effectType == "draw") {
                currentValue = effectJson.value("value", 0);
                if (this->upgraded_ && effectJson.contains("upgraded_value")) {
                    currentValue = effectJson.value("upgraded_value", currentValue);
                }
            } else {
                currentValue = effectJson.value("value", 0);
                if (this->upgraded_ && effectJson.contains("upgraded_value")) {
                    currentValue = effectJson.value("upgraded_value", currentValue);
                }
            }

            bool effectSuccess = false;
            if (effectType == "damage") {
                if (target_ == CardTarget::SINGLE_ENEMY) {
                    Enemy* enemy = combat->getEnemy(targetIndex);
                    if (enemy) {
                        int finalDamage = currentValue;
                        if (player && player->hasStatusEffect("weak")) {
                            finalDamage = static_cast<int>(std::round(finalDamage * 0.75));
                            LOG_DEBUG("card_onPlay", "Player is Weak, damage reduced to " + std::to_string(finalDamage));
                        }
                        enemy->takeDamage(finalDamage); 
                        effectSuccess = true;
                        if (!enemy->isAlive()) {
                            for (size_t idx = 0; idx < combat->getEnemyCount(); ++idx) {
                                if (combat->getEnemy(idx) == enemy) {
                                    combat->handleEnemyDeath(idx);
                                    break;
                                }
                            }
                            if (combat->areAllEnemiesDefeated()) {
                                if (!combat->isCombatOver()) {
                                    combat->end(true);
                                    if (auto game = combat->getGame()) game->endCombat(true);
                                }
                            }
                        }
                    }
                } else if (target_ == CardTarget::ALL_ENEMIES) {
                    bool anyEnemyDefeated = false;
                    for (size_t i = 0; i < combat->getEnemyCount(); ++i) {
                        Enemy* enemy = combat->getEnemy(i);
                        if (enemy && enemy->isAlive()) {
                            int finalDamage = currentValue;
                            if (player && player->hasStatusEffect("weak")) {
                                finalDamage = static_cast<int>(std::round(finalDamage * 0.75));
                                LOG_DEBUG("card_onPlay", "Player is Weak (AOE), damage reduced to " + std::to_string(finalDamage) + " for enemy " + enemy->getName());
                            }
                            enemy->takeDamage(finalDamage);
                            if (!enemy->isAlive()) {
                                combat->handleEnemyDeath(i);
                                anyEnemyDefeated = true;
                            }
                        }
                    }
                    if (anyEnemyDefeated && combat->areAllEnemiesDefeated()) {
                        if (!combat->isCombatOver()) {
                            combat->end(true);
                            if (auto game = combat->getGame()) game->endCombat(true);
                        }
                    }
                    effectSuccess = true;
                }
            } else if (effectType == "block") {
                if (player) {
                    player->addBlock(currentValue);
                    effectSuccess = true;
                }
            } else if (effectType == "apply_vulnerable" || effectType == "apply_weak" || effectType == "gain_strength") {
                std::string statusId = effectType.substr(std::string("apply_").length());
                 if (effectType == "gain_strength") statusId = "strength";

                if (target_ == CardTarget::SINGLE_ENEMY && (statusId == "vulnerable" || statusId == "weak")) {
                    Enemy* enemy = combat->getEnemy(targetIndex);
                    if (enemy && enemy->isAlive()) {
                        enemy->addStatusEffect(statusId, currentValue);
                        effectSuccess = true; 
                    } else if (enemy && !enemy->isAlive()) {
                        LOG_DEBUG("card_onPlay", "Target for " + statusId + " (" + getName() + ") is dead. Effect considered vacuously successful.");
                        effectSuccess = true; 
                    } else if (!enemy) {
                        LOG_DEBUG("card_onPlay", "Target enemy for " + statusId + " (" + getName() + ") does not exist (targetIndex=" + std::to_string(targetIndex) + "). Effect considered vacuously successful as combat might have ended.");
                        effectSuccess = true; 
                    }
                } else if (target_ == CardTarget::SELF && statusId == "strength") {
                     if(player) {
                        player->addStatusEffect(statusId, currentValue);
                        effectSuccess = true;
                     }
                }
            } else if (effectType == "draw") {
                if (player) {
                    player->drawCards(currentValue);
                    effectSuccess = true;
                }
            } else if (effectType == "status_effect") { 
                std::string statusId = effectJson.value("effect", "");
                std::string actualEffectTargetType = effectJson.value("target", "self"); 
                
                currentValue = effectJson.value("value", 0);
                if (this->upgraded_ && effectJson.contains("upgraded_value")) {
                    currentValue = effectJson.value("upgraded_value", currentValue);
                }

                if (statusId.empty()) {
                    LOG_WARNING("card_onPlay", "status_effect type missing 'effect' field in JSON for card '" + getId() + "'");
                } else {
                    if (actualEffectTargetType == "enemy" || actualEffectTargetType == "SINGLE_ENEMY") {
                        if (target_ == CardTarget::SINGLE_ENEMY) {
                            Enemy* enemy = combat->getEnemy(targetIndex);
                            if (enemy && enemy->isAlive()) {
                                enemy->addStatusEffect(statusId, currentValue);
                                effectSuccess = true;
                            } else if (enemy && !enemy->isAlive()) {
                                LOG_DEBUG("card_onPlay", "Target for generic status_effect '" + statusId + "' (" + getName() + ") is dead. Effect considered vacuously successful.");
                                effectSuccess = true;
                            } else if (!enemy) {
                                LOG_DEBUG("card_onPlay", "Target enemy for generic status_effect '" + statusId + "' (" + getName() + ") does not exist. Effect considered vacuously successful as combat might have ended.");
                                effectSuccess = true;
                            }
                        } else if (target_ == CardTarget::ALL_ENEMIES) {
                             for (size_t i = 0; i < combat->getEnemyCount(); ++i) {
                                Enemy* enemy = combat->getEnemy(i);
                                if (enemy && enemy->isAlive()) {
                                    enemy->addStatusEffect(statusId, currentValue);
                                }
                            }
                            effectSuccess = true; 
                        }
                    } else if (actualEffectTargetType == "self" || actualEffectTargetType == "SELF") {
                        if (player) {
                            player->addStatusEffect(statusId, currentValue);
                            effectSuccess = true;
                        }
                    } else if (actualEffectTargetType == "all_enemies" || actualEffectTargetType == "ALL_ENEMIES") {
                    for (size_t i = 0; i < combat->getEnemyCount(); ++i) {
                        Enemy* enemy = combat->getEnemy(i);
                        if (enemy && enemy->isAlive()) {
                                enemy->addStatusEffect(statusId, currentValue);
                        }
                    }
                        effectSuccess = true; 
                    }
                }
            }
            
            if (!effectSuccess && effectType != "") {
                LOG_WARNING("card_onPlay", "Effect type '" + effectType + "' for card '" + getId() + "' failed or not handled.");
                overallSuccess = false;
            }
        }
        
        return overallSuccess;
    } catch (const std::exception& e) {
        LOG_ERROR("card_onPlay", "Error processing card effects for " + getId() + ": " + std::string(e.what()));
        return fallbackCardEffect(player, targetIndex, combat);
    }
}

bool Card::fallbackCardEffect(Player* player, int targetIndex, Combat* combat) {
    if (type_ == CardType::ATTACK) {
        if (target_ == CardTarget::SINGLE_ENEMY) {
            Enemy* enemy = combat->getEnemy(targetIndex);
            if (enemy) {
                int damage = upgraded_ ? 9 : 6;
                enemy->takeDamage(damage);
                return true;
            }
        } else if (target_ == CardTarget::ALL_ENEMIES) {
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
        if (target_ == CardTarget::SELF) {
            int block = upgraded_ ? 8 : 5;
            player->addBlock(block);
            return true;
        }
    }
    
    return false;
}

bool Card::needsTarget() const {
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