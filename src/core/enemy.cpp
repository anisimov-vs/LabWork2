#include "core/enemy.h"
#include "core/player.h"
#include "core/combat.h"
#include "util/logger.h"

#include <random>
#include <chrono>
#include <iostream>

namespace deckstiny {

Enemy::Enemy(const std::string& id, const std::string& name, int maxHealth)
    : Character(id, name, maxHealth, 0), elite_(false), boss_(false), minGold_(10), maxGold_(20) {
}

const Intent& Enemy::getIntent() const {
    return currentIntent_;
}

void Enemy::setIntent(const Intent& intent) {
    currentIntent_ = intent;
}

bool Enemy::isElite() const {
    return elite_;
}

void Enemy::setElite(bool elite) {
    elite_ = elite;
}

bool Enemy::isBoss() const {
    return boss_;
}

void Enemy::setBoss(bool boss) {
    boss_ = boss;
}

int Enemy::getMinGold() const {
    return minGold_;
}

int Enemy::getMaxGold() const {
    return maxGold_;
}

void Enemy::setGoldReward(int min, int max) {
    minGold_ = min;
    maxGold_ = max;
}

int Enemy::rollGoldReward() const {
    if (!isAlive()) {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        std::mt19937 gen(seed);
        std::uniform_int_distribution<> dist(minGold_, maxGold_);
        return dist(gen);
    }
    return 0;
}

void Enemy::chooseNextMove(Combat* combat, Player* player) {
    if (moves_.empty()) {
        LOG_ERROR("combat", "Enemy " + getName() + " has no moves available");
        // Set an unknown intent but don't crash the game
        currentIntent_.type = "unknown";
        currentIntent_.value = 0;
        currentIntent_.target = "player";
        return;
    }
    
    LOG_DEBUG("combat", getName() + " selecting move from " + std::to_string(moves_.size()) + " options");
    
    // The player's health might influence the enemy's strategy in a more advanced implementation
    int playerHealth = player ? player->getHealth() : 0;
    
    // Choose a random move
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> dist(0, moves_.size() - 1);
    int moveIndex = dist(gen);
    
    // Set intent based on move
    // In a more advanced implementation, the move might depend on the combat state
    (void)combat; // Mark as intentionally unused for now
    (void)playerHealth; // Mark as intentionally unused for now
    std::string moveId = moves_[moveIndex];
    
    LOG_DEBUG("combat", "Selected move: " + moveId + " for enemy " + getName());
    
    // Look up the intent for this move in our stored intents
    auto it = moveIntents_.find(moveId);
    if (it != moveIntents_.end()) {
        // Use the stored intent information
        currentIntent_ = it->second;
        LOG_DEBUG("combat", "Set intent to type=" + currentIntent_.type + 
                 ", value=" + std::to_string(currentIntent_.value) + 
                 ", target=" + currentIntent_.target + 
                 (currentIntent_.effect.empty() ? "" : ", effect=" + currentIntent_.effect));
    } else {
        // This should never happen if loadFromJson validated correctly
        LOG_ERROR("combat", "No intent found for move " + moveId + " on enemy " + getName());
        currentIntent_.type = "unknown";
        currentIntent_.value = 0;
        currentIntent_.target = "player";
    }
}

void Enemy::takeTurn(Combat* combat, Player* player) {
    if (!isAlive() || !player) {
        return;
    }
    
    // In a more advanced implementation, the combat state might influence the enemy's action
    (void)combat; // Mark as intentionally unused for now
    
    // Execute move based on intent
    if (currentIntent_.type == "attack") {
        // Deal damage to player
        player->takeDamage(currentIntent_.value);
    } else if (currentIntent_.type == "attack_defend") {
        // Deal damage to player and gain block
        player->takeDamage(currentIntent_.value);
        addBlock(currentIntent_.secondaryValue);
    } else if (currentIntent_.type == "buff") {
        // Apply buff to self based on effect
        if (!currentIntent_.effect.empty()) {
            addStatusEffect(currentIntent_.effect, currentIntent_.value);
        }
        
        // Also add block if secondary value is specified
        if (currentIntent_.secondaryValue > 0) {
            addBlock(currentIntent_.secondaryValue);
        }
    } else if (currentIntent_.type == "defend") {
        // Gain block
        addBlock(currentIntent_.value);
    } else if (currentIntent_.type == "debuff") {
        // Apply debuff to player based on effect
        if (!currentIntent_.effect.empty()) {
            player->addStatusEffect(currentIntent_.effect, currentIntent_.value);
        }
    } else {
        std::cerr << "Warning: Unknown intent type '" << currentIntent_.type 
                  << "' for enemy " << getName() << std::endl;
    }
}

const std::vector<std::string>& Enemy::getPossibleMoves() const {
    return moves_;
}

void Enemy::addPossibleMove(const std::string& moveId) {
    if (std::find(moves_.begin(), moves_.end(), moveId) == moves_.end()) {
        moves_.push_back(moveId);
    }
}

void Enemy::startTurn() {
    Character::startTurn();
    
    // Enemy block should persist between turns, so don't reset block here
    // Process enemy-specific start of turn effects
}

void Enemy::endTurn() {
    Character::endTurn();
    
    // Process enemy-specific end of turn effects
}

bool Enemy::loadFromJson(const nlohmann::json& json) {
    if (!Character::loadFromJson(json)) {
        LOG_ERROR("enemy", "Failed to load character data for enemy " + getId());
        return false;
    }
    
    try {
        if (json.contains("is_elite")) {
            elite_ = json["is_elite"].get<bool>();
        }
        
        if (json.contains("is_boss")) {
            boss_ = json["is_boss"].get<bool>();
        }
        
        if (json.contains("min_gold")) {
            minGold_ = json["min_gold"].get<int>();
        }
        
        if (json.contains("max_gold")) {
            maxGold_ = json["max_gold"].get<int>();
        }
        
        LOG_INFO("enemy", "Created enemy: " + getName() + " with " + std::to_string(getHealth()) + "/" + 
                std::to_string(getMaxHealth()) + " HP");
        
        // Clear existing moves and intents
        moveIntents_.clear();
        moves_.clear();
        
        // Load moves - this is required for enemies
        if (!json.contains("moves") || !json["moves"].is_array() || json["moves"].empty()) {
            LOG_ERROR("enemy", "Enemy " + getId() + " has no moves defined in JSON");
            return false;
        }
        
        LOG_DEBUG("enemy", "Loading " + std::to_string(json["moves"].size()) + " moves for enemy " + getId());
            
        for (const auto& move : json["moves"]) {
            if (!move.contains("id")) {
                LOG_ERROR("enemy", "Move without ID found in enemy " + getId());
                return false;
            }
                
            std::string moveId = move["id"].get<std::string>();
            addPossibleMove(moveId);
            LOG_DEBUG("enemy", "Added move: " + moveId + " for enemy " + getId());
                
            // Store the intent information for this move
            if (!move.contains("intent")) {
                LOG_ERROR("enemy", "Move " + moveId + " has no intent defined for enemy " + getId());
                return false;
            }
                
            Intent intent;
            const auto& intentData = move["intent"];
                
            if (!intentData.contains("type")) {
                LOG_ERROR("enemy", "Intent for move " + moveId + " has no type defined");
                return false;
            }
            intent.type = intentData["type"].get<std::string>();
                
            if (intentData.contains("value")) {
                intent.value = intentData["value"].get<int>();
            }
                
            if (intentData.contains("secondary_value")) {
                intent.secondaryValue = intentData["secondary_value"].get<int>();
            }
                
            if (intentData.contains("target")) {
                intent.target = intentData["target"].get<std::string>();
            }
                
            if (intentData.contains("effect")) {
                intent.effect = intentData["effect"].get<std::string>();
            }
                
            // Store the intent for this move
            moveIntents_[moveId] = intent;
            LOG_DEBUG("enemy", "Added intent for move " + moveId + ": type=" + intent.type + 
                      ", value=" + std::to_string(intent.value) + 
                      ", target=" + intent.target + 
                      (intent.secondaryValue > 0 ? ", secondary_value=" + std::to_string(intent.secondaryValue) : "") + 
                      (intent.effect.empty() ? "" : ", effect=" + intent.effect));
        }
        
        LOG_INFO("enemy", "Enemy " + getId() + " loaded " + std::to_string(moves_.size()) + 
                 " moves and " + std::to_string(moveIntents_.size()) + " intents");
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("enemy", "Error loading enemy from JSON: " + std::string(e.what()));
        return false;
    }
}

std::unique_ptr<Entity> Enemy::clone() const {
    auto enemy = std::make_unique<Enemy>(getId(), getName(), getMaxHealth());
    enemy->setElite(elite_);
    enemy->setBoss(boss_);
    enemy->setGoldReward(minGold_, maxGold_);
    
    // Copy moves
    for (const auto& move : moves_) {
        enemy->addPossibleMove(move);
    }
    
    // Copy move intents
    enemy->moveIntents_ = moveIntents_;
    
    // Copy character state
    enemy->setHealth(getHealth());
    enemy->addBlock(getBlock());
    
    // Copy status effects
    for (const auto& effect : getStatusEffects()) {
        enemy->addStatusEffect(effect.first, effect.second);
    }
    
    return enemy;
}

std::shared_ptr<Enemy> Enemy::cloneEnemy() const {
    auto enemy = std::make_shared<Enemy>(getId(), getName(), getMaxHealth());
    enemy->setElite(elite_);
    enemy->setBoss(boss_);
    enemy->setGoldReward(minGold_, maxGold_);
    
    // Copy moves
    for (const auto& move : moves_) {
        enemy->addPossibleMove(move);
    }
    
    // Copy move intents
    enemy->moveIntents_ = moveIntents_;
    
    // Copy character state
    enemy->setHealth(getHealth());
    enemy->addBlock(getBlock());
    
    // Copy status effects
    for (const auto& effect : getStatusEffects()) {
        enemy->addStatusEffect(effect.first, effect.second);
    }
    
    return enemy;
}

bool Enemy::executeMove(const std::string& moveId, Combat* combat, Player* player) {
    if (!player) {
        return false;
    }
    
    // In a more advanced implementation, the combat state might influence the move execution
    (void)combat; // Mark as intentionally unused for now
    
    // Execute specific move
    if (moveId == "chomp") {
        // Deal 11 damage to player
        player->takeDamage(11);
        return true;
    } else if (moveId == "thrash") {
        // Deal 7 damage to player and gain 5 block
        player->takeDamage(7);
        addBlock(5);
        return true;
    } else if (moveId == "bellow") {
        // Gain 3 strength and 6 block
        addStatusEffect("strength", 3);
        addBlock(6);
        return true;
    }
    
    return false;
}

} // namespace deckstiny 