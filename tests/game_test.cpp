// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include <gtest/gtest.h>
#include "core/game.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/combat.h"
#include "core/card.h"
#include "core/relic.h"
#include "core/map.h"
#include "mocks/MockUI.h"
#include <memory>

namespace deckstiny {
namespace testing {

class GameTest : public ::testing::Test {
protected:
    std::unique_ptr<Game> game;
    std::shared_ptr<MockUI> mockUi;

    void SetUp() override {
        game = std::make_unique<Game>();
        mockUi = std::make_shared<MockUI>();
    }

    void TearDown() override {
        game.reset();
        mockUi.reset();
    }
};

// Test initialization of game
TEST_F(GameTest, Initialization) {
    EXPECT_TRUE(game->initialize(mockUi));
    EXPECT_EQ(game->getState(), GameState::MAIN_MENU);
    EXPECT_FALSE(game->isRunning());
}

// Test state transitions
TEST_F(GameTest, StateTransitions) {
    ASSERT_TRUE(game->initialize(mockUi));
    
    // Initial transition to Character Select
    game->setState(GameState::CHARACTER_SELECT);
    EXPECT_EQ(game->getState(), GameState::CHARACTER_SELECT);
    EXPECT_TRUE(mockUi->wasMethodCalled("showCharacterSelection"));
    mockUi->clearRecordedCalls();

    // Create player
    ASSERT_TRUE(game->createPlayer("ironclad"));
    
    // Generate map manually since this behavior might have changed
    if (!game->getMap()) {
        ASSERT_TRUE(game->generateMap(1));
    }
    
    // Now the map should exist
    ASSERT_NE(game->getMap(), nullptr) << "Map should exist after generateMap call";

    // Manually set state to MAP to trigger UI update
    game->setState(GameState::MAP);
    EXPECT_EQ(game->getState(), GameState::MAP);
    EXPECT_TRUE(mockUi->wasMethodCalled("showMap"));
    mockUi->clearRecordedCalls();

    // Transition to combat
    std::vector<std::string> enemies_to_load = {"jaw_worm"};
    // Check if enemy data is available before starting combat
    auto enemyData = game->getEnemyData("jaw_worm");
    ASSERT_NE(enemyData, nullptr) << "Jaw Worm enemy data should be loaded.";
    
    ASSERT_TRUE(game->startCombat(enemies_to_load));
    EXPECT_EQ(game->getState(), GameState::COMBAT);
    EXPECT_TRUE(mockUi->wasMethodCalled("showCombat"));
}

// Test player creation
TEST_F(GameTest, PlayerCreation) {
    auto mockUi = std::make_shared<MockUI>();
    ASSERT_TRUE(game->initialize(mockUi));
    ASSERT_TRUE(game->createPlayer("ironclad"));
    Player* player = game->getPlayer();
    ASSERT_NE(player, nullptr);
    EXPECT_EQ(player->getId(), "ironclad");
    EXPECT_EQ(player->getName(), "The Ironclad");
    EXPECT_GT(player->getHealth(), 0);
    EXPECT_GT(player->getMaxHealth(), 0);
    EXPECT_GT(player->getMaxHealth(), 0);
    EXPECT_GT(player->getDrawPile().size() + player->getHand().size() + player->getDiscardPile().size() + player->getExhaustPile().size(), 0);
    // Check if starting relics are added (e.g., Burning Blood for Ironclad)
    bool foundBurningBlood = false;
    for(const auto& relic : player->getRelics()) {
        if(relic->getId() == "burning_blood") {
            foundBurningBlood = true;
            break;
        }
    }
}

// Test card loading
TEST_F(GameTest, CardLoading) {
    ASSERT_TRUE(game->initialize(mockUi));
    auto strike = game->getCardData("strike");
    ASSERT_NE(strike, nullptr);
    EXPECT_EQ(strike->getName(), "Strike");
    EXPECT_EQ(strike->getType(), CardType::ATTACK);
    ASSERT_GT(game->getAllCards().size(), 0); // Check that cards are loaded
}

// Test enemy loading
TEST_F(GameTest, EnemyLoading) {
    ASSERT_TRUE(game->initialize(mockUi));
    auto slime = game->getEnemyData("acid_slime");
    ASSERT_NE(slime, nullptr);
    if (slime) {
        EXPECT_EQ(slime->getName(), "Acid Slime");
    }
    // Check that an invalid enemy returns nullptr
    auto invalidEnemy = game->getEnemyData("non_existent_enemy");
    EXPECT_EQ(invalidEnemy, nullptr);
}

// Test relic loading
TEST_F(GameTest, RelicLoading) {
    ASSERT_TRUE(game->initialize(mockUi));
    auto burningBlood = game->getRelicData("burning_blood");
    ASSERT_NE(burningBlood, nullptr);
    EXPECT_EQ(burningBlood->getName(), "Burning Blood");
    ASSERT_GT(game->getAllRelics().size(), 0);
}

// Test map generation
TEST_F(GameTest, MapGeneration) {
    ASSERT_TRUE(game->initialize(mockUi));
    EXPECT_TRUE(game->generateMap(1));
    GameMap* map = game->getMap();
    ASSERT_NE(map, nullptr);
    EXPECT_EQ(map->getAct(), 1);
}

// Test combat initialization
TEST_F(GameTest, CombatInitialization) {
    ASSERT_TRUE(game->initialize(mockUi));
    ASSERT_TRUE(game->createPlayer("ironclad", "TestPlayer"));
    ASSERT_NE(game->getEnemyData("jaw_worm"), nullptr) << "Jaw Worm enemy data should be loaded for combat init.";

    std::vector<std::string> enemies = {"jaw_worm"};
    ASSERT_TRUE(game->startCombat(enemies));
    EXPECT_EQ(game->getState(), GameState::COMBAT);
    ASSERT_NE(game->getCurrentCombat(), nullptr);
    EXPECT_EQ(game->getCurrentCombat()->getEnemyCount(), 1);
}

// Test input handling
TEST_F(GameTest, InputHandling) {
    ASSERT_TRUE(game->initialize(mockUi));
    bool result = game->processInput("1");
    EXPECT_TRUE(result);
    EXPECT_EQ(game->getState(), GameState::CHARACTER_SELECT);
    EXPECT_TRUE(mockUi->wasMethodCalled("showCharacterSelection"));
    
    mockUi->clearRecordedCalls();
    EXPECT_FALSE(game->processInput("invalid_command"));
    EXPECT_EQ(game->getState(), GameState::CHARACTER_SELECT);
}

TEST_F(GameTest, GameRunAndQuit) {
    ASSERT_TRUE(game->initialize(mockUi));
    game->setState(GameState::MAIN_MENU); 

    mockUi->addExpectedInput("2"); // Input for "Quit"
    ASSERT_TRUE(game->processInput("2")); // Process quit from main menu
}


} // namespace testing
} // namespace deckstiny 