// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include "core/combat.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/card.h"
#include "core/game.h"
#include "util/logger.h"

#include <algorithm>
#include <iostream>

namespace deckstiny {

Combat::Combat() 
    : player_(nullptr), game_(nullptr), turn_(0), playerTurn_(true), inCombat_(false) {
}

Combat::Combat(Player* player) 
    : player_(player), game_(nullptr), turn_(0), playerTurn_(true), inCombat_(false) {
}

void Combat::setPlayer(Player* player) {
    player_ = player;
}

Player* Combat::getPlayer() const {
    return player_;
}

void Combat::setGame(Game* game) {
    game_ = game;
}

Game* Combat::getGame() const {
    return game_;
}

void Combat::addEnemy(std::shared_ptr<Enemy> enemy) {
    if (enemy) {
        enemies_.push_back(enemy);
    }
}

const std::vector<std::shared_ptr<Enemy>>& Combat::getEnemies() const {
    return enemies_;
}

size_t Combat::getEnemyCount() const {
    return enemies_.size();
}

Enemy* Combat::getEnemy(size_t index) const {
    if (index < enemies_.size()) {
        return enemies_[index].get();
    }
    return nullptr;
}

bool Combat::areAllEnemiesDefeated() const {
    for (const auto& enemy : enemies_) {
        if (enemy->isAlive()) {
            return false;
        }
    }
    return !enemies_.empty();
}

bool Combat::isPlayerDefeated() const {
    return player_ && !player_->isAlive();
}

bool Combat::isCombatOver() const {
    return !inCombat_ || areAllEnemiesDefeated() || isPlayerDefeated();
}

int Combat::getTurn() const {
    return turn_;
}

bool Combat::isPlayerTurn() const {
    return playerTurn_;
}

void Combat::start() {
    if (!player_ || enemies_.empty()) {
        return;
    }
    
    LOG_INFO("combat", "Combat starting with " + std::to_string(enemies_.size()) + " enemies.");
    inCombat_ = true;
    playerTurn_ = true; 
    turn_ = 1;

    if (player_) {
        player_->setCurrentCombat(this);
        LOG_INFO("combat", "Calling player->beginCombat() for turn " + std::to_string(turn_));
    }
    
    while (!delayedActions_.empty()) {
        delayedActions_.pop();
    }
    
    for (auto& enemy : enemies_) {
        enemy->chooseNextMove(this, player_);
    }
    
    playerTurn_ = true;
    
    LOG_INFO("combat", "Combat initialization complete. Player turn: " + 
             std::string(playerTurn_ ? "true" : "false"));
}

void Combat::beginPlayerTurn() {
    if (!inCombat_ || !player_) {
        return;
    }
    
    LOG_INFO("combat", "Beginning player turn " + std::to_string(turn_));
    
    playerTurn_ = true;
    
    if (turn_ > 1) {
        LOG_INFO("combat", "Calling player->startTurn() for turn " + std::to_string(turn_));
        player_->startTurn();
        
        LOG_INFO("combat", "After startTurn - Hand size: " + std::to_string(player_->getHand().size()) + 
                 ", Draw pile size: " + std::to_string(player_->getDrawPile().size()) + 
                 ", Discard pile size: " + std::to_string(player_->getDiscardPile().size()));
    } else {
        LOG_INFO("combat", "Skipping player->startTurn() for first turn");
    }
    
    processDelayedActions();
}

void Combat::endPlayerTurn() {
    if (!inCombat_ || !player_) {
        return;
    }
    
    player_->endTurn();
    
    processEnemyTurns();
    
    turn_++;
    beginPlayerTurn();
}

void Combat::processEnemyTurns() {
    if (!inCombat_ || !player_) {
        return;
    }
    
    playerTurn_ = false;
    
    for (auto& enemy : enemies_) {
        if (enemy->isAlive()) {
            enemy->startTurn();
            
            enemy->takeTurn(this, player_);
            
            enemy->endTurn();
            
            if (isPlayerDefeated()) {
                LOG_INFO("combat", "Player was defeated by enemy: " + enemy->getName());
                end(false);
                
                if (game_) {
                    LOG_INFO("combat", "Game pointer available, calling game->endCombat(false)");
                    game_->endCombat(false);
                } else {
                    LOG_ERROR("combat", "Player defeated but game pointer not available!");
                }
                return;
            }
        }
    }
    
    if (player_) {
        LOG_DEBUG("combat", "Resetting player block at end of turn");
        player_->resetBlock();
    }
    
    for (auto& enemy : enemies_) {
        if (enemy->isAlive()) {
            enemy->chooseNextMove(this, player_);
        }
    }
    
    processDelayedActions();
    
    if (areAllEnemiesDefeated()) {
        end(true);
    }
}

bool Combat::playCard(int cardIndex, int targetIndex) {
    if (!inCombat_ || !player_ || !playerTurn_) {
        LOG_ERROR("combat", "Cannot play card: combat not active, player is null, or not player's turn");
        return false;
    }
    
    const auto& hand = player_->getHand();
    
    if (cardIndex < 0 || cardIndex >= static_cast<int>(hand.size())) {
        LOG_ERROR("combat", "Invalid card index: " + std::to_string(cardIndex) + ", hand size: " + std::to_string(hand.size()));
        return false;
    }
    
    Card* card = hand[cardIndex].get();
    LOG_DEBUG("combat", "Attempting to play card: " + card->getName());
    
    if (!card->canPlay(player_, targetIndex, this)) {
        LOG_INFO("combat", "Card cannot be played: " + card->getName());
        return false;
    }
    
    bool success = card->play(player_, targetIndex, this);
    LOG_DEBUG("combat", "Card played: " + card->getName() + ", success: " + (success ? "true" : "false"));
    
    LOG_DEBUG("combat", "After playing card - Hand size: " + std::to_string(player_->getHand().size()) + 
              ", Discard pile size: " + std::to_string(player_->getDiscardPile().size()));
    
    if (areAllEnemiesDefeated()) {
        LOG_INFO("combat", "All enemies defeated, ending combat");
        end(true);
    } else if (isPlayerDefeated()) {
        LOG_INFO("combat", "Player defeated, ending combat");
        end(false);
    }
    
    return success;
}

void Combat::addDelayedAction(std::function<void()> action, int delay, int priority, const std::string& source) {
    if (!action) {
        return;
    }
    
    CombatAction delayedAction;
    delayedAction.action = action;
    delayedAction.delay = delay;
    delayedAction.priority = priority;
    delayedAction.source = source;
    
    delayedActions_.push(delayedAction);
}

void Combat::processDelayedActions() {
    while (!delayedActions_.empty() && delayedActions_.top().delay <= 0) {
        CombatAction action = delayedActions_.top();
        delayedActions_.pop();
        
        if (action.action) {
            action.action();
        }
    }
    
    std::vector<CombatAction> remainingActions;
    while (!delayedActions_.empty()) {
        CombatAction action = delayedActions_.top();
        delayedActions_.pop();
        
        action.delay--;
        remainingActions.push_back(action);
    }
    
    for (const auto& action : remainingActions) {
        delayedActions_.push(action);
    }
}

void Combat::handleEnemyDeath(size_t index) {
    if (index >= enemies_.size()) {
        return;
    }
    
    if (areAllEnemiesDefeated()) {
        end(true);
    }
}

void Combat::end(bool victorious) {
    if (!inCombat_) {
        LOG_DEBUG("combat", "Combat::end called when not in combat");
        return;
    }
    
    inCombat_ = false;
    
    LOG_INFO("combat", "Combat ended with " + std::string(victorious ? "victory" : "defeat"));
    
    if (!victorious && isPlayerDefeated() && game_) {
        LOG_INFO("combat", "Player defeated, ensuring game transition to game over");
    }
}

} // namespace deckstiny 