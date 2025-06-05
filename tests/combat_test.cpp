// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include <gtest/gtest.h>
#include "core/combat.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/card.h"
#include "core/game.h"
#include "ui/ui_interface.h"
#include "mocks/MockUI.h"
#include "util/logger.h"
#include <memory>

namespace deckstiny {
namespace testing {

class CombatTest : public ::testing::Test {
protected:
    void SetUp() override {
        game = std::make_unique<Game>();
        mockUi = std::make_shared<MockUI>();
        ASSERT_TRUE(game->initialize(mockUi));

        // Create a player for testing
        player = std::make_shared<Player>(
            "ironclad",           // id
            "Test Player",       // name
            75,                  // max health
            3,                   // base energy
            5                    // initial hand size
        );
        
        // Give the player some cards (loaded from game data)
        strikeCard = game->getCardData("strike");
        ASSERT_NE(strikeCard, nullptr) << "Strike card failed to load from game data";
        player->addCardToDeck(strikeCard->cloneCard());
        
        // Try to load defend card, if not available create a test defend card
        defendCard = game->getCardData("defend");
        if (!defendCard) {
            util::Logger::getInstance().log(util::LogLevel::Info, "test", "Creating test defend card as game data doesn't have one");
            defendCard = std::make_shared<Card>(
                "test_defend",
                "Test Defend",
                "Gain 5 Block.",
                CardType::SKILL,
                CardRarity::BASIC,
                CardTarget::SELF,
                1,
                true
            );
        }
        player->addCardToDeck(defendCard->cloneCard());
        
        enemy = std::make_shared<Enemy>(
            "test_louse_small",            // id
            "Small Louse",        // name
            15                   // health
        );
        
        // Create the combat instance
        combat = std::make_unique<Combat>(player.get()); // Construct with player
        combat->setGame(game.get()); // Set the game instance
        combat->addEnemy(enemy);

        // Initialize player for combat (draws initial hand, sets energy)
        player->beginCombat();
    }

    void TearDown() override {
        // Clean up resources
        combat.reset();
        player.reset();
        enemy.reset();
        strikeCard.reset();
        defendCard.reset();
        game.reset();
        mockUi.reset();
    }

    // Helper to find a card in hand by ID
    int findCardInHand(const std::string& cardId) {
        const auto& hand = player->getHand();
        for (size_t i = 0; i < hand.size(); ++i) {
            if (hand[i] && hand[i]->getId() == cardId) {
                return static_cast<int>(i);
            }
        }
        return -1;
    }

    std::unique_ptr<Game> game;
    std::shared_ptr<MockUI> mockUi;
    std::shared_ptr<Player> player;
    std::shared_ptr<Enemy> enemy;
    std::unique_ptr<Combat> combat;
    std::shared_ptr<Card> strikeCard;
    std::shared_ptr<Card> defendCard;
};

// Test combat initialization
TEST_F(CombatTest, Initialization) {
    // Verify the player was set correctly
    EXPECT_EQ(combat->getPlayer(), player.get());
    
    // Verify an enemy was added
    EXPECT_EQ(combat->getEnemyCount(), 1);
    EXPECT_EQ(combat->getEnemy(0), enemy.get());
    
    // Verify initial combat state (before combat->start())
    EXPECT_FALSE(combat->areAllEnemiesDefeated());
    EXPECT_FALSE(combat->isPlayerDefeated());

    combat->start(); // Start the combat

    // Verify combat state after start
    EXPECT_TRUE(combat->isInCombat());
    EXPECT_FALSE(combat->isCombatOver());
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

TEST_F(CombatTest, PlayerDealsDamageWithStrike) {
    combat->start(); // Start combat, player draws, enemy picks intent
    ASSERT_TRUE(player->getHand().size() > 0) << "Player should have cards in hand.";

    int strikeIdx = findCardInHand("strike");
    ASSERT_NE(strikeIdx, -1) << "Strike card not found in hand.";

    Card* cardToPlay = player->getHand()[strikeIdx].get();
    ASSERT_NE(cardToPlay, nullptr);
    ASSERT_EQ(cardToPlay->getId(), "strike");
    ASSERT_TRUE(cardToPlay->getCost() <= player->getEnergy()) << "Not enough energy for Strike";

    int enemyInitialHealth = enemy->getHealth();
    // From strike.json: "value": 6 for damage effect
    // From card.cpp, currentValue is sourced from effect's JSON value
    int expectedDamage = 6; 

    bool played = combat->playCard(strikeIdx, 0); // Play Strike on the first enemy
    ASSERT_TRUE(played) << "Failed to play Strike card. Energy: " << player->getEnergy() 
                        << ", Cost: " << cardToPlay->getCost();

    EXPECT_EQ(enemy->getHealth(), enemyInitialHealth - expectedDamage);
}

TEST_F(CombatTest, PlayerDealsLethalDamageWithStrike) {
    combat->start();
    ASSERT_TRUE(player->getHand().size() > 0);

    int strikeIdx = findCardInHand("strike");
    ASSERT_NE(strikeIdx, -1) << "Strike card not found in hand.";
    
    // From strike.json: "value": 6 for damage effect
    int strikeDamage = 6;
    enemy->setHealth(strikeDamage -1); // Set enemy health to be just below lethal
    int enemyInitialHealth = enemy->getHealth();
    ASSERT_LT(enemyInitialHealth, strikeDamage) << "Test setup error: enemy health not less than strike damage";

    bool played = combat->playCard(strikeIdx, 0);
    ASSERT_TRUE(played) << "Failed to play Strike card.";

    EXPECT_LE(enemy->getHealth(), 0);
    EXPECT_FALSE(enemy->isAlive());
    EXPECT_TRUE(combat->areAllEnemiesDefeated());
    EXPECT_TRUE(combat->isCombatOver());
}

TEST_F(CombatTest, PlayerPlaysUpgradedStrike) {
    player->clearDrawPile();
    player->clearDiscardPile();
    player->clearHand();

    std::shared_ptr<Card> freshStrike = game->getCardData("strike");
    ASSERT_TRUE(freshStrike->isUpgradable());
    freshStrike->upgrade();
    ASSERT_TRUE(freshStrike->isUpgraded());
    player->addCardToDeck(freshStrike); // Add the upgraded one

    player->startTurn();

    combat->start(); // Start combat to set up enemy intents and ensure player turn is active.

    int strikeIdx = findCardInHand("strike");
    ASSERT_NE(strikeIdx, -1) << "Upgraded Strike card not found in hand.";

    Card* cardToPlay = player->getHand()[strikeIdx].get();
    ASSERT_NE(cardToPlay, nullptr);
    ASSERT_TRUE(cardToPlay->isUpgraded()) << "Strike card in hand is not upgraded.";

    int enemyInitialHealth = enemy->getHealth();
    // From strike.json: "upgraded_value": 9 for damage effect
    int expectedUpgradedDamage = 9;

    bool played = combat->playCard(strikeIdx, 0);
    ASSERT_TRUE(played) << "Failed to play upgraded Strike card.";

    EXPECT_EQ(enemy->getHealth(), enemyInitialHealth - expectedUpgradedDamage);
}

} // namespace testing
} // namespace deckstiny 