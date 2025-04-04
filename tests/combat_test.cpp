#include <gtest/gtest.h>
#include "core/combat.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/card.h"
#include <memory>

namespace deckstiny {
namespace testing {

class CombatTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a player for testing
        player = std::make_shared<Player>(
            "player1",           // id
            "Test Player",       // name
            PlayerClass::IRONCLAD, // class
            80,                  // max health
            3,                   // base energy
            5                    // initial hand size
        );
        
        // Give the player some cards
        auto strike = std::make_shared<Card>(
            "strike",            // id
            "Strike",            // name
            "Deal 6 damage.",    // description
            CardType::ATTACK,    // type
            CardRarity::COMMON,  // rarity
            CardTarget::SINGLE_ENEMY, // target
            1,                   // cost
            true                 // upgradable
        );
        player->addCardToDeck(strike);
        
        auto defend = std::make_shared<Card>(
            "defend",            // id
            "Defend",            // name
            "Gain 5 Block.",     // description
            CardType::SKILL,     // type
            CardRarity::COMMON,  // rarity
            CardTarget::SELF,    // target
            1,                   // cost
            true                 // upgradable
        );
        player->addCardToDeck(defend);
        
        // Create a test enemy
        enemy = std::make_shared<Enemy>(
            "enemy1",            // id
            "Test Enemy",        // name
            40                   // health
        );
        
        // Create the combat instance
        combat = std::make_unique<Combat>(player.get());
        combat->addEnemy(enemy);
    }

    void TearDown() override {
        // Clean up resources
        combat.reset();
        player.reset();
        enemy.reset();
    }

    std::shared_ptr<Player> player;
    std::shared_ptr<Enemy> enemy;
    std::unique_ptr<Combat> combat;
};

// Test combat initialization
TEST_F(CombatTest, Initialization) {
    // Verify the player was set correctly
    EXPECT_EQ(combat->getPlayer(), player.get());
    
    // Verify an enemy was added
    EXPECT_EQ(combat->getEnemyCount(), 1);
    EXPECT_EQ(combat->getEnemy(0), enemy.get());
    
    // Verify initial combat state
    EXPECT_FALSE(combat->isCombatOver());
    EXPECT_FALSE(combat->areAllEnemiesDefeated());
    EXPECT_FALSE(combat->isPlayerDefeated());
}

// Test starting combat
TEST_F(CombatTest, Start) {
    // Start combat
    combat->start();
    
    // Verify combat has started
    EXPECT_TRUE(combat->isPlayerTurn());
    EXPECT_EQ(combat->getTurn(), 1);
    
    // Player should have drawn cards
    EXPECT_GT(player->getHand().size(), 0);
    
    // Enemy should have chosen a move
    EXPECT_FALSE(enemy->getIntent().type.empty());
}

// Test player turn mechanics
TEST_F(CombatTest, PlayerTurn) {
    combat->start();
    
    // Verify initial energy
    EXPECT_EQ(player->getEnergy(), 3);
    
    // If player has a card in hand, play it
    if (!player->getHand().empty()) {
        size_t initialHandSize = player->getHand().size();
        
        // Play the first card
        EXPECT_TRUE(combat->playCard(0, 0)); // target the enemy
        
        // Verify card was played (removed from hand)
        EXPECT_EQ(player->getHand().size(), initialHandSize - 1);
        
        // Verify energy was spent
        EXPECT_LT(player->getEnergy(), 3);
    }
    
    // End player turn
    combat->endPlayerTurn();
    
    // Verify turn ended
    EXPECT_EQ(combat->getTurn(), 2);
    EXPECT_TRUE(combat->isPlayerTurn()); // Should cycle back to player's turn
    
    // Player should have drawn new cards and refreshed energy
    EXPECT_GT(player->getHand().size(), 0);
    EXPECT_EQ(player->getEnergy(), 3);
}

// Test enemy behavior
TEST_F(CombatTest, EnemyBehavior) {
    combat->start();
    
    // End player turn to trigger enemy turn
    int playerHealthBefore = player->getHealth();
    combat->endPlayerTurn();
    
    // Check if enemy's action affected the player
    if (enemy->getIntent().type == "ATTACK") {
        // Player should have taken damage
        EXPECT_LT(player->getHealth(), playerHealthBefore);
    }
}

// Test combat resolution - player victory
TEST_F(CombatTest, PlayerVictory) {
    combat->start();
    
    // Deal damage to the enemy to defeat it
    enemy->takeDamage(enemy->getHealth());
    
    // Verify enemy is defeated
    EXPECT_FALSE(enemy->isAlive());
    EXPECT_TRUE(combat->areAllEnemiesDefeated());
    EXPECT_TRUE(combat->isCombatOver());
}

// Test combat resolution - player defeat
TEST_F(CombatTest, PlayerDefeat) {
    combat->start();
    
    // Deal damage to the player to defeat them
    player->takeDamage(player->getHealth());
    
    // Verify player is defeated
    EXPECT_FALSE(player->isAlive());
    EXPECT_TRUE(combat->isPlayerDefeated());
    EXPECT_TRUE(combat->isCombatOver());
}

// Test delayed actions
TEST_F(CombatTest, DelayedActions) {
    combat->start();
    
    // Add a delayed action
    bool actionExecuted = false;
    combat->addDelayedAction([&actionExecuted]() { actionExecuted = true; }, 0);
    
    // Process delayed actions
    combat->processDelayedActions();
    
    // Verify action was executed
    EXPECT_TRUE(actionExecuted);
    
    // Add a delayed action with delay
    bool delayedActionExecuted = false;
    combat->addDelayedAction([&delayedActionExecuted]() { delayedActionExecuted = true; }, 1);
    
    // Process delayed actions immediately (shouldn't execute)
    combat->processDelayedActions();
    EXPECT_FALSE(delayedActionExecuted);
    
    // End turn to reduce delay
    combat->endPlayerTurn();
    
    // Process delayed actions again (should execute now)
    combat->processDelayedActions();
    EXPECT_TRUE(delayedActionExecuted);
}

} // namespace testing
} // namespace deckstiny 