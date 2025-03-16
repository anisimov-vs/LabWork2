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
    
    // Add logging
    LOG_INFO("combat", "Starting combat with " + std::to_string(enemies_.size()) + " enemies");
    
    // Reset combat state
    turn_ = 1;
    playerTurn_ = true;
    inCombat_ = true;
    
    // Clear delayed actions
    while (!delayedActions_.empty()) {
        delayedActions_.pop();
    }
    
    // Initialize enemies
    for (auto& enemy : enemies_) {
        enemy->chooseNextMove(this, player_);
    }
    
    // No need to call beginPlayerTurn here - player already initialized in Game::startCombat
    // Note: We'll set player turn to true but won't draw more cards
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
    
    // Process player turn start - but only if it's not the first turn
    // On the first turn, the player has already been initialized by beginCombat
    if (turn_ > 1) {
        LOG_INFO("combat", "Calling player->startTurn() for turn " + std::to_string(turn_));
        player_->startTurn();
        
        // Log the state after starting turn
        LOG_INFO("combat", "After startTurn - Hand size: " + std::to_string(player_->getHand().size()) + 
                 ", Draw pile size: " + std::to_string(player_->getDrawPile().size()) + 
                 ", Discard pile size: " + std::to_string(player_->getDiscardPile().size()));
    } else {
        LOG_INFO("combat", "Skipping player->startTurn() for first turn");
    }
    
    // Process relics and other effects
    // TODO: Implement relic effects
    
    // Process delayed actions
    processDelayedActions();
}

void Combat::endPlayerTurn() {
    if (!inCombat_ || !player_) {
        return;
    }
    
    // Process player turn end
    player_->endTurn();
    
    // Process enemy turns
    processEnemyTurns();
    
    // Start next turn
    turn_++;
    beginPlayerTurn();
}

void Combat::processEnemyTurns() {
    if (!inCombat_ || !player_) {
        return;
    }
    
    playerTurn_ = false;
    
    // Process each enemy's turn
    for (auto& enemy : enemies_) {
        if (enemy->isAlive()) {
            // Handle enemy turn start - this will reset their block
            enemy->startTurn();
            
            // Execute enemy's action based on intent
            enemy->takeTurn(this, player_);
            
            // Handle enemy turn end
            enemy->endTurn();
            
            // Check if player is defeated
            if (isPlayerDefeated()) {
                LOG_INFO("combat", "Player was defeated by enemy: " + enemy->getName());
                end(false);
                
                // If game pointer is available, call endCombat directly
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
    
    // Reset player's block at the end of the turn (after enemy attacks)
    if (player_) {
        LOG_DEBUG("combat", "Resetting player block at end of turn");
        player_->resetBlock();
    }
    
    // Choose next moves for enemies
    for (auto& enemy : enemies_) {
        if (enemy->isAlive()) {
            enemy->chooseNextMove(this, player_);
        }
    }
    
    // Process delayed actions
    processDelayedActions();
    
    // Check if all enemies are defeated
    if (areAllEnemiesDefeated()) {
        end(true);
    }
}

bool Combat::playCard(int cardIndex, int targetIndex) {
    if (!inCombat_ || !player_ || !playerTurn_) {
        LOG_ERROR("combat", "Cannot play card: combat not active, player is null, or not player's turn");
        return false;
    }
    
    // Get player's hand
    const auto& hand = player_->getHand();
    
    // Check if card index is valid
    if (cardIndex < 0 || cardIndex >= static_cast<int>(hand.size())) {
        LOG_ERROR("combat", "Invalid card index: " + std::to_string(cardIndex) + ", hand size: " + std::to_string(hand.size()));
        return false;
    }
    
    // Get the card
    Card* card = hand[cardIndex].get();
    LOG_DEBUG("combat", "Attempting to play card: " + card->getName());
    
    // Check if card can be played
    if (!card->canPlay(player_, targetIndex, this)) {
        LOG_ERROR("combat", "Card cannot be played: " + card->getName());
        return false;
    }
    
    // Play the card
    bool success = card->play(player_, targetIndex, this);
    LOG_DEBUG("combat", "Card played: " + card->getName() + ", success: " + (success ? "true" : "false"));
    
    // Log the state after playing
    LOG_DEBUG("combat", "After playing card - Hand size: " + std::to_string(player_->getHand().size()) + 
              ", Discard pile size: " + std::to_string(player_->getDiscardPile().size()));
    
    // Check if combat is over
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
    // Process all actions with delay 0
    while (!delayedActions_.empty() && delayedActions_.top().delay <= 0) {
        CombatAction action = delayedActions_.top();
        delayedActions_.pop();
        
        // Execute the action
        if (action.action) {
            action.action();
        }
    }
    
    // Decrement delay for remaining actions
    std::vector<CombatAction> remainingActions;
    while (!delayedActions_.empty()) {
        CombatAction action = delayedActions_.top();
        delayedActions_.pop();
        
        action.delay--;
        remainingActions.push_back(action);
    }
    
    // Push back remaining actions
    for (const auto& action : remainingActions) {
        delayedActions_.push(action);
    }
}

void Combat::handleEnemyDeath(size_t index) {
    if (index >= enemies_.size()) {
        return;
    }
    
    // Process enemy death
    // TODO: Implement death effects
    
    // Check if all enemies are defeated
    if (areAllEnemiesDefeated()) {
        end(true);
    }
}

void Combat::calculateRewards() {
    // Calculate gold reward
    int goldReward = 0;
    for (const auto& enemy : enemies_) {
        goldReward += enemy->rollGoldReward();
    }
    
    // TODO: Use goldReward to give to player when implemented properly
    // For now, just print it to indicate it's being calculated
    std::cout << "Calculated gold reward: " << goldReward << std::endl;
    
    // TODO: Calculate card rewards
    
    // TODO: Calculate relic rewards
    
    // TODO: Return rewards
}

void Combat::end(bool victorious) {
    if (!inCombat_) {
        LOG_DEBUG("combat", "Combat::end called when not in combat");
        return;
    }
    
    inCombat_ = false;
    
    // Process combat end
    LOG_INFO("combat", "Combat ended with " + std::string(victorious ? "victory" : "defeat"));
    
    // If player is defeated and game pointer is available, ensure we call endCombat
    if (!victorious && isPlayerDefeated() && game_) {
        LOG_INFO("combat", "Player defeated, ensuring game transition to game over");
    }
}

} // namespace deckstiny 