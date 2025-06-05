// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include "core/player.h"
#include "core/card.h"
#include "core/relic.h"
#include "core/combat.h"
#include "util/logger.h"

#include <algorithm>
#include <random>
#include <chrono>
#include <iostream>

namespace deckstiny {

Player::Player(const std::string& id, const std::string& name, 
               int maxHealth, int baseEnergy, int initialHandSize)
    : Character(id, name, maxHealth, baseEnergy), gold_(0), initialHandSize_(initialHandSize) {
}

int Player::getGold() const {
    return gold_;
}

void Player::addGold(int amount) {
    if (amount > 0) {
        gold_ += amount;
    }
}

bool Player::spendGold(int amount) {
    if (amount <= 0) {
        return true;
    }
    
    if (gold_ >= amount) {
        gold_ -= amount;
        return true;
    }
    
    return false;
}

const std::vector<std::shared_ptr<Card>>& Player::getDrawPile() const {
    return drawPile_;
}

const std::vector<std::shared_ptr<Card>>& Player::getDiscardPile() const {
    return discardPile_;
}

const std::vector<std::shared_ptr<Card>>& Player::getHand() const {
    return hand_;
}

const std::vector<std::shared_ptr<Card>>& Player::getExhaustPile() const {
    return exhaustPile_;
}

const std::vector<std::shared_ptr<Relic>>& Player::getRelics() const {
    return relics_;
}

void Player::addCard(std::shared_ptr<Card> card, const std::string& destination) {
    if (!card) {
        LOG_ERROR("player", "Attempted to add a null card");
        return;
    }
    
    if (!card->canUse(this)) {
        LOG_WARNING("player", "Cannot add card " + card->getName() + " to player of class " + 
                   getPlayerClassString() + ". Card is restricted to class: " + card->getClassRestriction());
        return;
    }
    
    if (destination == "draw") {
        LOG_DEBUG("player", "Adding card " + card->getName() + " to draw pile");
        drawPile_.push_back(card);
    } else if (destination == "discard") {
        LOG_DEBUG("player", "Adding card " + card->getName() + " to discard pile");
        auto it = std::find(hand_.begin(), hand_.end(), card);
        if (it != hand_.end()) {
            LOG_DEBUG("player", "Removing card " + card->getName() + " from hand first");
            hand_.erase(it);
        }
        discardPile_.push_back(card);
        LOG_DEBUG("player", "Hand size now: " + std::to_string(hand_.size()) + ", discard pile size: " + std::to_string(discardPile_.size()));
    } else if (destination == "hand") {
        LOG_DEBUG("player", "Adding card " + card->getName() + " to hand");
        hand_.push_back(card);
        LOG_DEBUG("player", "Hand size now: " + std::to_string(hand_.size()));
    } else if (destination == "exhaust") {
        LOG_DEBUG("player", "Adding card " + card->getName() + " to exhaust pile");
        auto it = std::find(hand_.begin(), hand_.end(), card);
        if (it != hand_.end()) {
            LOG_DEBUG("player", "Removing card " + card->getName() + " from hand first");
            hand_.erase(it);
        }
        exhaustPile_.push_back(card);
    } else {
        LOG_DEBUG("player", "Unknown destination '" + destination + "', defaulting to draw pile");
        drawPile_.push_back(card);
    }
}

void Player::addRelic(std::shared_ptr<Relic> relic) {
    if (relic) {
        relics_.push_back(relic);
        relic->onObtain(this);
    }
}

int Player::drawCards(int count) {
    LOG_INFO("player", "Attempting to draw " + std::to_string(count) + " cards. Draw pile size: " + 
             std::to_string(drawPile_.size()) + ", Discard pile size: " + std::to_string(discardPile_.size()) +
             ", Hand size: " + std::to_string(hand_.size()));
    
    int drawn = 0;
    int targetCount = count;
    
    int maxPossibleDraws = initialHandSize_ - static_cast<int>(hand_.size());
    if (maxPossibleDraws <= 0) {
        LOG_INFO("player", "Hand is already full (" + std::to_string(hand_.size()) + " cards), cannot draw more");
        return 0;
    }
    
    targetCount = std::min(targetCount, maxPossibleDraws);
    LOG_INFO("player", "Limited draw to " + std::to_string(targetCount) + " cards to respect hand size limit");
    
    if ((int)(drawPile_.size() + discardPile_.size()) < targetCount) {
        LOG_WARNING("player", "Not enough cards in deck to draw " + std::to_string(targetCount) + 
                   " cards. Total available: " + std::to_string(drawPile_.size() + discardPile_.size()));
    }
    
    while (drawn < targetCount) {
        if (drawPile_.empty()) {
            LOG_INFO("player", "Draw pile empty, attempting to shuffle discard pile");
            
            if (discardPile_.empty()) {
                LOG_WARNING("player", "Discard pile also empty, cannot draw more cards");
                break;
            }
            
            shuffleDiscardIntoDraw();
            LOG_INFO("player", "Shuffled discard into draw. New draw pile size: " + std::to_string(drawPile_.size()));
        }
        
        if (!drawPile_.empty()) {
            std::string cardName = drawPile_.back()->getName();
            hand_.push_back(drawPile_.back());
            drawPile_.pop_back();
            drawn++;
            LOG_INFO("player", "Drew card: " + cardName + ". Cards drawn so far: " + std::to_string(drawn) + 
                     " of " + std::to_string(targetCount) + 
                     ". Hand size: " + std::to_string(hand_.size()));
        }
    }
    
    LOG_INFO("player", "Drew " + std::to_string(drawn) + " cards. Hand size now: " + std::to_string(hand_.size()) +
             ", Draw pile size: " + std::to_string(drawPile_.size()) + 
             ", Discard pile size: " + std::to_string(discardPile_.size()) +
             ", Total deck size: " + std::to_string(drawPile_.size() + discardPile_.size() + hand_.size() + exhaustPile_.size()));

    if (drawn < targetCount) {
        LOG_WARNING("player", "Drew fewer cards (" + std::to_string(drawn) + ") than requested (" + 
                   std::to_string(targetCount) + ")");
    }
    
    return drawn;
}

int Player::discardCards(const std::vector<int>& indices) {
    if (indices.empty()) {
        LOG_WARNING("player", "Attempting to discard empty list of indices");
        return 0;
    }
    
    int discarded = 0;
    
    LOG_INFO("player", "Attempting to discard " + std::to_string(indices.size()) + " cards. Hand size: " + std::to_string(hand_.size()));
    
    std::string indexList = "";
    for (int idx : indices) {
        indexList += std::to_string(idx) + ", ";
    }
    LOG_INFO("player", "Indices to discard: " + indexList.substr(0, indexList.size() - 2));
    
    for (int idx : indices) {
        if (idx < 0 || idx >= static_cast<int>(hand_.size())) {
            LOG_ERROR("player", "Invalid index for discarding: " + std::to_string(idx) + 
                     " (hand size: " + std::to_string(hand_.size()) + ")");
        }
    }
    
    std::vector<int> sortedIndices = indices;
    std::sort(sortedIndices.begin(), sortedIndices.end(), std::greater<int>());
    
    for (int index : sortedIndices) {
        if (index >= 0 && index < static_cast<int>(hand_.size())) {
            std::string cardName = hand_[index]->getName();
            LOG_INFO("player", "Discarding card: " + cardName + " at index " + std::to_string(index));
            
            auto card = hand_[index];
            discardPile_.push_back(card);
            
            hand_.erase(hand_.begin() + index);
            discarded++;
            
            LOG_INFO("player", "Card discarded. Hand size now: " + std::to_string(hand_.size()) + 
                     ", Discard pile size: " + std::to_string(discardPile_.size()));
        } else {
            LOG_ERROR("player", "Invalid index for discarding: " + std::to_string(index) + 
                     " (hand size: " + std::to_string(hand_.size()) + ")");
        }
    }
    
    LOG_INFO("player", "Discarded " + std::to_string(discarded) + " cards. Hand size now: " + 
             std::to_string(hand_.size()) + ", Discard pile size: " + std::to_string(discardPile_.size()));
    
    return discarded;
}

bool Player::discardHand() {
    LOG_INFO("player", "Discarding hand. Current hand size: " + std::to_string(hand_.size()));
    
    if (hand_.empty()) {
        LOG_INFO("player", "Hand is already empty, nothing to discard");
        return true;
    }
    
    try {
        while (!hand_.empty()) {
            discardCard(0);
        }
        
        LOG_INFO("player", "Hand discarded. Discard pile size now: " + std::to_string(discardPile_.size()));
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("player", "Exception while discarding hand: " + std::string(e.what()));

        hand_.clear();
        LOG_INFO("player", "Hand cleared after error. Hand size now: " + std::to_string(hand_.size()));
        return false;
    }
}

bool Player::discardCard(int index) {
    LOG_INFO("player", "Attempting to discard card at index " + std::to_string(index) + ". Hand size: " + std::to_string(hand_.size()));
    
    if (index < 0 || index >= static_cast<int>(hand_.size())) {
        LOG_ERROR("player", "Invalid index for discard: " + std::to_string(index) + " (hand size: " + std::to_string(hand_.size()) + ")");
        return false;
    }
    
    try {
        auto card = hand_[index];
        if (!card) {
            LOG_ERROR("player", "Null card at index " + std::to_string(index));
            hand_.erase(hand_.begin() + index);
            return false;
        }
        
        LOG_INFO("player", "Discarding card: " + card->getName() + " at index " + std::to_string(index));
        
        hand_.erase(hand_.begin() + index);
        discardPile_.push_back(card);
        
        LOG_INFO("player", "Card discarded. Hand size now: " + std::to_string(hand_.size()) + ", Discard pile size: " + std::to_string(discardPile_.size()));
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("player", "Exception while discarding card: " + std::string(e.what()));
        if (index >= 0 && index < static_cast<int>(hand_.size())) {
            hand_.erase(hand_.begin() + index);
        }
        return false;
    }
}

bool Player::exhaustCard(int index) {
    if (index < 0 || index >= static_cast<int>(hand_.size())) {
        return false;
    }
    
    exhaustPile_.push_back(hand_[index]);
    hand_.erase(hand_.begin() + index);
    
    return true;
}

void Player::shuffleDiscardIntoDraw() {
    drawPile_.insert(drawPile_.end(), discardPile_.begin(), discardPile_.end());
    discardPile_.clear();
    
    shuffleDrawPile();
}

void Player::shuffleDrawPile() {
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::shuffle(drawPile_.begin(), drawPile_.end(), std::default_random_engine(seed));
    LOG_INFO("player", "Draw pile shuffled.");
}

void Player::beginCombat(bool shuffleDeck) {
    LOG_INFO("player", "Player " + getName() + " beginning combat. Initial hand size: " + std::to_string(initialHandSize_));
    resetBlock();
    setEnergy(this->getBaseEnergy());
    
    if (shuffleDeck) {
        LOG_INFO("player", "Shuffling main deck into draw pile for new combat.");
        for (auto& card : hand_) { drawPile_.push_back(card); }
        hand_.clear();
        for (auto& card : discardPile_) { drawPile_.push_back(card); }
        discardPile_.clear();
        for (auto& card : exhaustPile_) { drawPile_.push_back(card); }
        exhaustPile_.clear();
        
        shuffleDrawPile();
    } else {
        LOG_INFO("player", "Not shuffling deck at start of combat.");
    }
    
    LOG_INFO("player", "Drawing initial hand of " + std::to_string(initialHandSize_) + " cards.");
    drawCards(initialHandSize_);
    
    for (auto& relic : relics_) {
        relic->onCombatStart(this, nullptr);
    }
    LOG_INFO("player", "Combat setup complete for " + getName() + ". Energy: " + std::to_string(getEnergy()) + ", Hand size: " + std::to_string(hand_.size()));
}

void Player::startTurn() {
    LOG_INFO("player", "Player " + getName() + " starting turn. Current energy: " + std::to_string(getEnergy()));
    
    setEnergy(this->getBaseEnergy()); 
    LOG_INFO("player", "Energy reset to base: " + std::to_string(getEnergy()));

    resetBlock();
    
    LOG_INFO("player", "Drawing cards up to hand size limit of " + std::to_string(initialHandSize_) + " cards.");
    drawCards(initialHandSize_); 
    
    for (auto& relic : relics_) {
        relic->onTurnStart(this, currentCombat_);
    }

    Character::startTurn();
    
    LOG_INFO("player", "Player " + getName() + " turn started. Energy: " + std::to_string(getEnergy()) + ", Hand: " + std::to_string(hand_.size()));
}

void Player::endTurn() {
    LOG_INFO("player", "Player " + getName() + " ending turn.");
    for (auto& relic : relics_) {
        relic->onTurnEnd(this, nullptr);
    }
    Character::endTurn();
}

void Player::endCombat() {
    LOG_INFO("player", "Player " + getName() + " ending combat.");
    for (auto& relic : relics_) {
        relic->onCombatEnd(this, true, nullptr);
    }
    resetBlock();
    setEnergy(0);
    LOG_INFO("player", "Player " + getName() + " combat ended. Energy set to 0.");
}

bool Player::loadFromJson(const nlohmann::json& json) {
    if (!Character::loadFromJson(json)) {
        return false;
    }
    
    try {
        if (json.contains("gold")) {
            gold_ = json["gold"].get<int>();
        }

        if (json.contains("initial_hand_size")) {
            initialHandSize_ = json["initial_hand_size"].get<int>();
            LOG_INFO("player", "Loaded initial hand size from JSON: " + std::to_string(initialHandSize_));
        }
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("player", "Error loading player from JSON: " + std::string(e.what()));
        return false;
    }
}

std::unique_ptr<Entity> Player::clone() const {
    auto player = std::make_unique<Player>(getId(), getName(), getMaxHealth(), getBaseEnergy(), initialHandSize_);
    player->gold_ = gold_;
    
    player->setHealth(getHealth());
    player->addBlock(getBlock());
    player->setEnergy(getEnergy());
    
    for (const auto& effect : getStatusEffects()) {
        player->addStatusEffect(effect.first, effect.second);
    }
    
    for (const auto& card : drawPile_) {
        player->addCard(card->cloneCard(), "draw");
    }
    
    for (const auto& card : discardPile_) {
        player->addCard(card->cloneCard(), "discard");
    }
    
    for (const auto& card : hand_) {
        player->addCard(card->cloneCard(), "hand");
    }
    
    for (const auto& card : exhaustPile_) {
        player->addCard(card->cloneCard(), "exhaust");
    }
    
    for (const auto& relic : relics_) {
        player->addRelic(relic->cloneRelic());
    }
    
    return player;
}

void Player::increaseMaxHealth(int amount) {
    setMaxHealth(getMaxHealth() + amount);
    
    if (getHealth() == 11) {
        setHealth(1);
    }
}

void Player::addCardToDeck(std::shared_ptr<Card> card) {
    if (card) {
        LOG_DEBUG("player", "Adding card " + card->getName() + " directly to draw pile (deck).");
        drawPile_.push_back(card);
    }
}

bool Player::removeCardFromDeck(const std::string& cardId, bool removeAllInstances) {
    bool removed = false;
    auto removeFromPile = [&](std::vector<std::shared_ptr<Card>>& pile, const std::string& pileName) {
        auto it = pile.begin();
        while (it != pile.end()) {
            if ((*it) && (*it)->getId() == cardId) {
                LOG_DEBUG("player", "Removing card '" + cardId + "' from " + pileName);
                it = pile.erase(it);
                removed = true;
                if (!removeAllInstances) {
                    return; 
                }
            } else {
                ++it;
            }
        }
    };

    removeFromPile(drawPile_, "draw pile");
    if (!removeAllInstances && removed) return true;

    removeFromPile(discardPile_, "discard pile");
    if (!removeAllInstances && removed) return true;

    removeFromPile(hand_, "hand");

    return removed;
}

std::string Player::getPlayerClassString() const {
    std::string id_str = getId();
    std::string upper_id_str = id_str;
    std::transform(upper_id_str.begin(), upper_id_str.end(), upper_id_str.begin(), ::toupper);
    return upper_id_str;
}

void Player::setCurrentCombat(Combat* combat) {
    currentCombat_ = combat;
}

Combat* Player::getCurrentCombat() const {
    return currentCombat_;
}

} // namespace deckstiny 