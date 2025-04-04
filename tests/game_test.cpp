#include <gtest/gtest.h>
#include "core/game.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/combat.h"
#include "core/card.h"
#include "core/relic.h"
#include "core/map.h"
#include "ui/text_ui.h"
#include <memory>

namespace deckstiny {
namespace testing {

class GameTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a game instance for testing
        game = std::make_unique<Game>();
        ui = std::make_shared<TextUI>();
    }

    void TearDown() override {
        // Clean up resources
        game.reset();
        ui.reset();
    }

    std::unique_ptr<Game> game;
    std::shared_ptr<TextUI> ui;
};

// Test initialization of game
TEST_F(GameTest, Initialization) {
    // Test that initialization succeeds
    EXPECT_TRUE(game->initialize(ui));
    
    // Test initial state is set to MAIN_MENU
    EXPECT_EQ(game->getState(), GameState::MAIN_MENU);
    
    // Test that game is not initially running
    EXPECT_FALSE(game->isRunning());
}

// Test state transitions
TEST_F(GameTest, StateTransitions) {
    ASSERT_TRUE(game->initialize(ui));
    
    // Test state transition to CHARACTER_SELECT
    game->setState(GameState::CHARACTER_SELECT);
    EXPECT_EQ(game->getState(), GameState::CHARACTER_SELECT);
    
    // Test state transition to MAP
    game->setState(GameState::MAP);
    EXPECT_EQ(game->getState(), GameState::MAP);
    
    // Test state transition to COMBAT
    game->setState(GameState::COMBAT);
    EXPECT_EQ(game->getState(), GameState::COMBAT);
}

// Test player creation
TEST_F(GameTest, PlayerCreation) {
    ASSERT_TRUE(game->initialize(ui));
    
    // Test creating a player with the Ironclad class
    EXPECT_TRUE(game->createPlayer("ironclad", "TestPlayer"));
    
    // Verify the player was created
    Player* player = game->getPlayer();
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->getName(), "TestPlayer");
    
    // Test that player has starting deck
    EXPECT_GT(player->getDrawPile().size() + player->getDiscardPile().size() + player->getHand().size(), 0);
}

// Test card loading
TEST_F(GameTest, CardLoading) {
    ASSERT_TRUE(game->initialize(ui));
    
    // Test loading a specific card
    auto strike = game->loadCard("strike");
    ASSERT_NE(strike, nullptr);
    EXPECT_EQ(strike->getName(), "Strike");
    EXPECT_EQ(strike->getType(), CardType::ATTACK);
    
    // Test loading all cards
    EXPECT_TRUE(game->loadAllCards());
}

// Test enemy loading
TEST_F(GameTest, EnemyLoading) {
    ASSERT_TRUE(game->initialize(ui));
    
    // Test loading a specific enemy
    auto slime = game->loadEnemy("small_slime");
    ASSERT_NE(slime, nullptr);
    EXPECT_EQ(slime->getName(), "Small Slime");
    
    // Test loading all enemies
    EXPECT_TRUE(game->loadAllEnemies());
}

// Test relic loading
TEST_F(GameTest, RelicLoading) {
    ASSERT_TRUE(game->initialize(ui));
    
    // Test loading a specific relic
    auto burningBlood = game->loadRelic("burning_blood");
    ASSERT_NE(burningBlood, nullptr);
    EXPECT_EQ(burningBlood->getName(), "Burning Blood");
    
    // Test loading all relics
    EXPECT_TRUE(game->loadAllRelics());
}

// Test map generation
TEST_F(GameTest, MapGeneration) {
    ASSERT_TRUE(game->initialize(ui));
    
    // Test generating a map for act 1
    EXPECT_TRUE(game->generateMap(1));
    
    // Verify the map was created
    GameMap* map = game->getMap();
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(map->getAct(), 1);
}

// Test combat initialization
TEST_F(GameTest, CombatInitialization) {
    ASSERT_TRUE(game->initialize(ui));
    ASSERT_TRUE(game->createPlayer("ironclad", "TestPlayer"));
    
    // Test starting combat with specific enemies
    std::vector<std::string> enemies = {"small_slime"};
    EXPECT_TRUE(game->startCombat(enemies));
    
    // Verify combat was started
    EXPECT_EQ(game->getState(), GameState::COMBAT);
    ASSERT_NE(game->getCurrentCombat(), nullptr);
    
    // Verify combat has the correct enemies
    EXPECT_EQ(game->getCurrentCombat()->getEnemyCount(), 1);
}

// Test input handling
TEST_F(GameTest, InputHandling) {
    ASSERT_TRUE(game->initialize(ui));
    
    // Test handling input in main menu
    game->setState(GameState::MAIN_MENU);
    EXPECT_TRUE(game->processInput("1")); // New game
    EXPECT_EQ(game->getState(), GameState::CHARACTER_SELECT);
    
    // Test invalid input
    EXPECT_FALSE(game->processInput("invalid_command"));
}

} // namespace testing
} // namespace deckstiny 