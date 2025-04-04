#include "core/relic.h"
#include "core/player.h"
#include "core/combat.h"

#include <iostream>

namespace deckstiny {

Relic::Relic(const std::string& id, const std::string& name, const std::string& description,
             RelicRarity rarity, const std::string& flavorText)
    : Entity(id, name), description_(description), flavorText_(flavorText), 
      rarity_(rarity), counter_(0) {
}

const std::string& Relic::getDescription() const {
    return description_;
}

void Relic::setDescription(const std::string& description) {
    description_ = description;
}

const std::string& Relic::getFlavorText() const {
    return flavorText_;
}

void Relic::setFlavorText(const std::string& flavorText) {
    flavorText_ = flavorText;
}

RelicRarity Relic::getRarity() const {
    return rarity_;
}

int Relic::getCounter() const {
    return counter_;
}

void Relic::setCounter(int counter) {
    counter_ = counter;
}

int Relic::incrementCounter(int amount) {
    counter_ += amount;
    return counter_;
}

void Relic::resetCounter() {
    counter_ = 0;
}

void Relic::onObtain(Player* player) {
    // Mark parameter as used
    (void)player;
    // Default implementation does nothing
}

void Relic::onCombatStart(Player* player, Combat* combat) {
    // Mark parameters as used
    (void)player;
    (void)combat;
    // Default implementation does nothing
}

void Relic::onTurnStart(Player* player, Combat* combat) {
    // Mark parameters as used
    (void)player;
    (void)combat;
    // Default implementation does nothing
}

void Relic::onTurnEnd(Player* player, Combat* combat) {
    // Mark parameters as used
    (void)player;
    (void)combat;
    // Default implementation does nothing
}

int Relic::onTakeDamage(Player* player, int damage, Combat* combat) {
    // Mark parameters as used
    (void)player;
    (void)combat;
    // Default implementation doesn't modify damage
    return damage;
}

int Relic::onDealDamage(Player* player, int damage, int targetIndex, Combat* combat) {
    // Mark parameters as used
    (void)player;
    (void)targetIndex;
    (void)combat;
    // Default implementation doesn't modify damage
    return damage;
}

void Relic::onCombatEnd(Player* player, bool victorious, Combat* combat) {
    // Mark parameter as used
    (void)combat;
    
    // Default implementation does nothing
    // For Burning Blood, this would heal the player
    if (getId() == "burning_blood" && victorious && player) {
        player->heal(6);
    }
}

bool Relic::loadFromJson(const nlohmann::json& json) {
    if (!Entity::loadFromJson(json)) {
        return false;
    }
    
    try {
        if (json.contains("description")) {
            description_ = json["description"].get<std::string>();
        }
        
        if (json.contains("flavor_text")) {
            flavorText_ = json["flavor_text"].get<std::string>();
        }
        
        if (json.contains("rarity")) {
            std::string rarityStr = json["rarity"].get<std::string>();
            if (rarityStr == "STARTER") {
                rarity_ = RelicRarity::STARTER;
            } else if (rarityStr == "COMMON") {
                rarity_ = RelicRarity::COMMON;
            } else if (rarityStr == "UNCOMMON") {
                rarity_ = RelicRarity::UNCOMMON;
            } else if (rarityStr == "RARE") {
                rarity_ = RelicRarity::RARE;
            } else if (rarityStr == "BOSS") {
                rarity_ = RelicRarity::BOSS;
            } else if (rarityStr == "SHOP") {
                rarity_ = RelicRarity::SHOP;
            } else if (rarityStr == "EVENT") {
                rarity_ = RelicRarity::EVENT;
            }
        }
        
        if (json.contains("counter")) {
            counter_ = json["counter"].get<int>();
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Error loading relic from JSON: " << e.what() << std::endl;
        return false;
    }
}

std::unique_ptr<Entity> Relic::clone() const {
    auto relic = std::make_unique<Relic>(getId(), getName(), description_, rarity_, flavorText_);
    relic->counter_ = counter_;
    return relic;
}

std::shared_ptr<Relic> Relic::cloneRelic() const {
    return std::make_shared<Relic>(getId(), getName(), description_, rarity_, flavorText_);
}

} // namespace deckstiny 