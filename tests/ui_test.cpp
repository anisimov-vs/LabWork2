#include <gtest/gtest.h>
#include "mocks/MockUI.h"
#include "core/game.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/combat.h"
#include "core/event.h"
#include "core/card.h"
#include "core/relic.h"
#include "core/map.h"
#include <memory>
#include <nlohmann/json.hpp>

namespace deckstiny {
namespace testing {

class UITest : public ::testing::Test {
protected:
    std::shared_ptr<Game> game;
    std::shared_ptr<MockUI> mockUi;
    std::shared_ptr<Player> player;
    std::shared_ptr<Enemy> enemy;
    std::shared_ptr<Card> card;
    std::shared_ptr<Relic> relic;
    std::shared_ptr<Event> event;
    std::shared_ptr<Combat> combat;
    std::shared_ptr<GameMap> map;

    void SetUp() override {
        game = std::make_shared<Game>();
        mockUi = std::make_shared<MockUI>();
        
        mockUi->initialize(game.get());
        game->initialize(mockUi);

        player = std::make_shared<Player>("ironclad", "Test Player", 75, 3, 5);
        enemy = std::make_shared<Enemy>("enemy1", "Test Enemy", 50);
        card = std::make_shared<Card>(
            "strike", "Strike", "Deal 6 damage.", CardType::ATTACK, 
            CardRarity::COMMON, CardTarget::SINGLE_ENEMY, 1, true);
        player->addCardToDeck(card);
        player->shuffleDrawPile();
        relic = std::make_shared<Relic>(
            "burning_blood", "Burning Blood", "Heal 6 HP.", RelicRarity::STARTER);
        player->addRelic(relic);
        
        event = std::make_shared<Event>();
        nlohmann::json eventJson = {
            {"id", "test_event"}, {"name", "Test Event"}, {"description", "This is a test event"},
            {"choices", nlohmann::json::array({
                {{"text", "Choice 1"}, {"resultText", "You chose option 1."}},
                {{"text", "Choice 2"}, {"resultText", "You chose option 2."}}
            })}
        };
        event->loadFromJson(eventJson);
        
        combat = std::make_shared<Combat>(player.get());
        combat->addEnemy(enemy);
        map = std::make_shared<GameMap>();
        map->generate(1);
    }
    
    void TearDown() override {
        mockUi->clearRecordedCalls();
    }
};

TEST_F(UITest, Initialization) {
    EXPECT_TRUE(mockUi->wasMethodCalled("initialize"));
}

TEST_F(UITest, ShowMainMenu) {
    mockUi->showMainMenu();
    EXPECT_TRUE(mockUi->wasMethodCalled("showMainMenu"));
    EXPECT_EQ(mockUi->getLastShownState(), GameState::MAIN_MENU);
}

TEST_F(UITest, ShowCombat) {
    Player player("ironclad", "Test Player", 80, 3, 5);
    Combat combat(&player);
    mockUi->showCombat(&combat);
    EXPECT_TRUE(mockUi->wasMethodCalled("showCombat"));
    EXPECT_EQ(mockUi->getLastShownState(), GameState::COMBAT);
    ASSERT_NE(mockUi->lastCombat_, nullptr);
}

TEST_F(UITest, ShowEvent) {
    Event mockEvent("test_event", "Mock Event", "Description");
    Player mockPlayer;
    mockUi->showEvent(&mockEvent, &mockPlayer);

    EXPECT_TRUE(mockUi->wasMethodCalled("showEvent"));
    EXPECT_EQ(mockUi->getLastShownState(), GameState::EVENT);
    ASSERT_NE(mockUi->lastShownEvent_, nullptr);
    if (mockUi->lastShownEvent_) {
        EXPECT_EQ(mockUi->lastShownEvent_->getId(), "test_event");
}
}

TEST_F(UITest, ShowMap) {
    game->createPlayer("ironclad", "TestPlayerGame");
    game->generateMap(1);
    game->setState(GameState::MAP);

    EXPECT_TRUE(mockUi->wasMethodCalled("showMap"));
    EXPECT_EQ(mockUi->getLastShownState(), GameState::MAP);
}

TEST_F(UITest, ShowMessage) {
    mockUi->showMessage("Test message", false);
    EXPECT_TRUE(mockUi->wasMethodCalled("showMessage"));
    ASSERT_FALSE(mockUi->getDisplayedMessagesHistory().empty());
    EXPECT_EQ(mockUi->getLastMessageText(), "Test message");
    }
    
TEST_F(UITest, Shutdown) {
    mockUi->shutdown();
    EXPECT_TRUE(mockUi->wasMethodCalled("shutdown"));
    }
    
TEST(MockUITest, InputHandling) {
    auto mockUi = std::make_shared<MockUI>();
    bool callbackCalled = false;
    std::string receivedInput;

    mockUi->setInputCallback([&](const std::string& input) {
        callbackCalled = true;
        receivedInput = input;
        return true;
    });

    mockUi->addExpectedInput("test_input");
    mockUi->triggerNextInput();

    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(receivedInput, "test_input");
}

TEST_F(UITest, TriggerInput_UICallback) {
    std::string receivedInput;
    bool callbackCalled = false;
    mockUi->setInputCallback([&](const std::string& input) {
        receivedInput = input;
        callbackCalled = true;
        return true;
    });
    mockUi->addExpectedInput("test_input_ui");
    bool processed = mockUi->triggerNextInput();

    EXPECT_TRUE(processed);
    EXPECT_TRUE(callbackCalled);
    EXPECT_EQ(receivedInput, "test_input_ui");
}

TEST_F(UITest, GetInputPrompt) {
    mockUi->addExpectedInput("user_response");
    std::string response = mockUi->getInput("Enter your name:");
    EXPECT_EQ(mockUi->lastInputPrompt_, "Enter your name:");
    EXPECT_EQ(response, "user_response");
    EXPECT_TRUE(mockUi->wasMethodCalled("getInput"));
}

TEST_F(UITest, OriginalTriggerInputLogic) {
    mockUi->setInputCallback([&](const std::string& input) {
        if (input == "goto_map_input") {
            game->setState(GameState::MAP);
            return true;
        }
        return false;
    });
    mockUi->addExpectedInput("goto_map_input");
    mockUi->triggerNextInput();
    if (mockUi->wasMethodCalled("showMap")) {
        EXPECT_EQ(mockUi->getLastShownState(), GameState::MAP);
    }
}

} // namespace testing
} // namespace deckstiny 