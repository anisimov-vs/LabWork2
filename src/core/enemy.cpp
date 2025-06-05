// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include "core/enemy.h"
#include "core/player.h"
#include "core/combat.h"
#include "core/game.h"
#include "util/logger.h"

#include <random>
#include <chrono>
#include <iostream>
#include <sstream>

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

        currentIntent_.type = "unknown";
        currentIntent_.value = 0;
        currentIntent_.target = "player";
        return;
    }
    
    LOG_DEBUG("combat", getName() + " selecting move from " + std::to_string(moves_.size()) + " options");
    
    int playerHealth = player ? player->getHealth() : 0;
    
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 gen(seed);
    std::uniform_int_distribution<> dist(0, moves_.size() - 1);
    int moveIndex = dist(gen);
    
    (void)combat;
    (void)playerHealth;
    std::string moveId = moves_[moveIndex];
    
    LOG_DEBUG("combat", "Selected move: " + moveId + " for enemy " + getName());
    
    auto it = moveIntents_.find(moveId);
    if (it != moveIntents_.end()) {
        currentIntent_ = it->second;
        LOG_DEBUG("combat", "Set intent to type=" + currentIntent_.type + 
                 ", value=" + std::to_string(currentIntent_.value) + 
                 ", target=" + currentIntent_.target + 
                 (currentIntent_.effect.empty() ? "" : ", effect=" + currentIntent_.effect));
    } else {
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
    
    (void)combat;
    
    if (currentIntent_.type == "attack") {
        int finalDamage = currentIntent_.value;
        if (hasStatusEffect("weak")) {
            finalDamage = static_cast<int>(std::round(finalDamage * 0.75));
            LOG_DEBUG("combat", getName() + " is Weak, attack damage reduced to " + std::to_string(finalDamage));
        }
        player->takeDamage(finalDamage);
    } else if (currentIntent_.type == "attack_defend") {
        int finalDamage = currentIntent_.value;
        if (hasStatusEffect("weak")) {
            finalDamage = static_cast<int>(std::round(finalDamage * 0.75));
            LOG_DEBUG("combat", getName() + " is Weak, attack_defend damage reduced to " + std::to_string(finalDamage));
        }
        player->takeDamage(finalDamage);
        addBlock(currentIntent_.secondaryValue);
    } else if (currentIntent_.type == "buff") {
        if (!currentIntent_.effect.empty()) {
            addStatusEffect(currentIntent_.effect, currentIntent_.value);
        }
        
        if (currentIntent_.secondaryValue > 0) {
            addBlock(currentIntent_.secondaryValue);
        }
    } else if (currentIntent_.type == "defend") {
        addBlock(currentIntent_.value);
    } else if (currentIntent_.type == "debuff") {
        if (!currentIntent_.effect.empty()) {
            player->addStatusEffect(currentIntent_.effect, currentIntent_.value);
        }
    } else if (currentIntent_.type == "summon") {
        LOG_DEBUG("combat", getName() + " is summoning. Intent value: " + std::to_string(currentIntent_.value));
        std::string summonType = "";
        int numToSummon = currentIntent_.value;

        if (currentIntent_.associatedEffectsJson.is_array()) {
            for (const auto& effectJson : currentIntent_.associatedEffectsJson) {
                if (effectJson.is_object() && effectJson.value("type", "") == "summon") {
                    summonType = effectJson.value("summon_type", "");
                    break; 
                }
            }
        }

        if (!summonType.empty() && numToSummon > 0 && combat && combat->getGame()) {
            LOG_INFO("combat", getName() + " attempts to summon " + std::to_string(numToSummon) + " of type '" + summonType + "'");
            for (int i = 0; i < numToSummon; ++i) {
                std::shared_ptr<Enemy> summonedEnemy = combat->getGame()->loadEnemy(summonType);
                if (summonedEnemy) {
                    combat->addEnemy(summonedEnemy);
                    LOG_INFO("combat", "Successfully summoned a " + summonType + ". Total enemies: " + std::to_string(combat->getEnemyCount()));
                } else {
                    LOG_ERROR("combat", "Failed to load enemy type '" + summonType + "' for summoning.");
                }
            }
        } else {
            LOG_WARNING("combat", getName() + " summon failed. SummonType: '" + summonType + "', NumToSummon: " + std::to_string(numToSummon) + ", Combat valid: " + (combat ? "true":"false") + ", Game valid: " + (combat && combat->getGame() ? "true":"false"));
        }
    } else if (currentIntent_.type == "attack_debuff") {
        int finalDamage = currentIntent_.value;
        if (hasStatusEffect("weak")) {
            finalDamage = static_cast<int>(std::round(finalDamage * 0.75));
            LOG_DEBUG("combat", getName() + " is Weak, attack_debuff damage reduced to " + std::to_string(finalDamage));
        }
        if (player) player->takeDamage(finalDamage);
        if (player && !currentIntent_.effect.empty() && currentIntent_.secondaryValue > 0) {
            player->addStatusEffect(currentIntent_.effect, currentIntent_.secondaryValue);
            LOG_DEBUG("combat", getName() + " applied debuff '" + currentIntent_.effect + "' for " + std::to_string(currentIntent_.secondaryValue) + " turns to player.");
        }
    } else if (currentIntent_.type == "defend_debuff") {
        addBlock(currentIntent_.value);
        if (player && !currentIntent_.effect.empty() && currentIntent_.secondaryValue > 0) {
            player->addStatusEffect(currentIntent_.effect, currentIntent_.secondaryValue);
            LOG_DEBUG("combat", getName() + " applied debuff '" + currentIntent_.effect + "' for " + std::to_string(currentIntent_.secondaryValue) + " turns to player while defending.");
        }
    } else {
        LOG_WARNING("combat", "Unknown intent type '" + currentIntent_.type + "' for enemy " + getName());
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
    
}

void Enemy::endTurn() {
    Character::endTurn();
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
        
        moveIntents_.clear();
        moves_.clear();
        
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
                
            if (move.contains("effects") && move["effects"].is_array()) {
                intent.associatedEffectsJson = move["effects"];
            }
            
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
    
    for (const auto& move : moves_) {
        enemy->addPossibleMove(move);
    }
    
    enemy->moveIntents_ = moveIntents_;
    
    enemy->setHealth(getHealth());
    enemy->addBlock(getBlock());
    
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
    
    for (const auto& move : moves_) {
        enemy->addPossibleMove(move);
    }
    
    enemy->moveIntents_ = moveIntents_;
    
    enemy->setHealth(getHealth());
    enemy->addBlock(getBlock());
    
    for (const auto& effect : getStatusEffects()) {
        enemy->addStatusEffect(effect.first, effect.second);
    }
    
    return enemy;
}

std::string Enemy::getIntentDescription() const {
    std::stringstream ss;
    if (currentIntent_.type == "attack") {
        ss << "Attack: " << currentIntent_.value;
    } else if (currentIntent_.type == "attack_defend") {
        ss << "Attack: " << currentIntent_.value << ", Defend: " << currentIntent_.secondaryValue;
    } else if (currentIntent_.type == "defend") {
        ss << "Defend: " << currentIntent_.value;
    } else if (currentIntent_.type == "buff") {
        ss << "Buff";
        if (!currentIntent_.effect.empty()) {
            ss << " (" << currentIntent_.effect << " +" << currentIntent_.value << ")";
        }
    } else if (currentIntent_.type == "debuff") {
        ss << "Debuff";
        if (!currentIntent_.effect.empty()) {
            ss << " (" << currentIntent_.effect << " +" << currentIntent_.value << ")";
        }
    } else if (currentIntent_.type == "attack_debuff") {
        ss << "Attack: " << currentIntent_.value;
        if (!currentIntent_.effect.empty()) {
            ss << ", Debuff (" << currentIntent_.effect;
            if (currentIntent_.secondaryValue > 0) {
                ss << " " << currentIntent_.secondaryValue;
            }
            ss << ")";
        } else {
            ss << ", Debuff";
        }
    } else if (currentIntent_.type == "defend_debuff") {
        ss << "Defend: " << currentIntent_.value;
        if (!currentIntent_.effect.empty()) {
            ss << ", Debuff (" << currentIntent_.effect;
            if (currentIntent_.secondaryValue > 0) {
                ss << " " << currentIntent_.secondaryValue;
            }
            ss << ")";
        } else {
            ss << ", Debuff";
        }
    } else if (currentIntent_.type == "summon") {
        ss << "Summon";
         if (currentIntent_.value > 0) {
            ss << " (" << currentIntent_.value << ")";
        }
    } else {
        ss << currentIntent_.type;
        if (currentIntent_.value > 0) {
            ss << " (" << currentIntent_.value << ")";
        }
    }
    return ss.str();
}

} // namespace deckstiny 