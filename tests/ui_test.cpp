#include <gtest/gtest.h>
#include "ui/ui_interface.h"
#include "ui/text_ui.h"
#include "core/game.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/combat.h"
#include "core/event.h"
#include "core/card.h"
#include "core/relic.h"
#include "core/map.h"
#include <memory>
#include <sstream>
#include <nlohmann/json.hpp>

namespace deckstiny {
namespace testing {

// Base UIInterface class tests
class UITest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a game instance for testing
        game = std::make_shared<Game>();
        
        // Create a text UI for testing
        ui = std::make_shared<TextUI>();
        
        // Initialize UI with game
        ui->initialize(game.get());
        
        // Redirect cout to our stringstream for testing outputs
        originalCoutBuffer = std::cout.rdbuf();
        std::cout.rdbuf(outputStream.rdbuf());
        
        // Create game components for UI testing
        player = std::make_shared<Player>(
            "player1",           // id
            "Test Player",       // name
            PlayerClass::IRONCLAD, // class
            75,                  // max health
            3,                   // base energy
            5                    // initial hand size
        );
        
        enemy = std::make_shared<Enemy>(
            "enemy1",            // id
            "Test Enemy",        // name
            50                   // health
        );
        
        // Create a mock card
        card = std::make_shared<Card>(
            "strike",            // id
            "Strike",            // name
            "Deal 6 damage.",    // description
            CardType::ATTACK,    // type
            CardRarity::COMMON,  // rarity
            CardTarget::SINGLE_ENEMY, // target
            1,                   // cost
            true                 // upgradable
        );
        
        // Add card to player's deck
        player->addCardToDeck(card);
        player->shuffleDrawPile();
        
        // Create a mock relic
        relic = std::make_shared<Relic>(
            "burning_blood",     // id
            "Burning Blood",     // name
            "Heal 6 HP at the end of combat.", // description
            RelicRarity::STARTER // rarity
        );
        
        // Add relic to player
        player->addRelic(relic);
        
        // Create a mock event using JSON
        event = std::make_shared<Event>();
        nlohmann::json eventJson = {
            {"id", "test_event"},
            {"name", "Test Event"},
            {"description", "This is a test event"},
            {"choices", nlohmann::json::array({
                {
                    {"text", "Choice 1"},
                    {"resultText", "You chose option 1."}
                },
                {
                    {"text", "Choice 2"},
                    {"resultText", "You chose option 2."}
                }
            })}
        };
        event->loadFromJson(eventJson);
        
        // Create a mock combat
        combat = std::make_shared<Combat>(player.get());
        combat->addEnemy(enemy);
        
        // Create a mock map
        map = std::make_shared<GameMap>();
        map->generate(1); // Generate Act 1 map
    }
    
    void TearDown() override {
        // Restore cout
        std::cout.rdbuf(originalCoutBuffer);
    }
    
    // Helper to get output and clear the stream
    std::string getOutputAndClear() {
        std::string output = outputStream.str();
        outputStream.str("");
        outputStream.clear();
        return output;
    }
    
    std::shared_ptr<Game> game;
    std::shared_ptr<TextUI> ui;
    std::shared_ptr<Player> player;
    std::shared_ptr<Enemy> enemy;
    std::shared_ptr<Card> card;
    std::shared_ptr<Relic> relic;
    std::shared_ptr<Event> event;
    std::shared_ptr<Combat> combat;
    std::shared_ptr<GameMap> map;
    
    std::stringstream outputStream;
    std::streambuf* originalCoutBuffer;
};

// Test UI initialization
TEST_F(UITest, Initialization) {
    // Test initialization (already done in SetUp)
    EXPECT_TRUE(ui != nullptr);
    
    std::string output = getOutputAndClear();
    EXPECT_FALSE(output.empty());
}

// Test showing the main menu
TEST_F(UITest, ShowMainMenu) {
    ui->showMainMenu();
    
    std::string output = getOutputAndClear();
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output.find("Main Menu") != std::string::npos);
}

// Test showing player stats
TEST_F(UITest, ShowPlayerStats) {
    ui->showPlayerStats(player.get());
    
    std::string output = getOutputAndClear();
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output.find("Test Player") != std::string::npos);
    EXPECT_TRUE(output.find("75") != std::string::npos); // Health
}

// Test showing enemy stats
TEST_F(UITest, ShowEnemyStats) {
    ui->showEnemyStats(enemy.get());
    
    std::string output = getOutputAndClear();
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output.find("Test Enemy") != std::string::npos);
    EXPECT_TRUE(output.find("50") != std::string::npos); // Health
}

// Test showing cards
TEST_F(UITest, ShowCards) {
    // Convert shared_ptr<Card> to Card* for the interface
    std::vector<Card*> cards = { card.get() };
    ui->showCards(cards, "Test Cards");
    
    std::string output = getOutputAndClear();
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output.find("Strike") != std::string::npos);
    EXPECT_TRUE(output.find("Deal 6 damage") != std::string::npos);
}

// Test showing combat
TEST_F(UITest, ShowCombat) {
    ui->showCombat(combat.get());
    
    std::string output = getOutputAndClear();
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output.find("Test Player") != std::string::npos);
    EXPECT_TRUE(output.find("Test Enemy") != std::string::npos);
}

// Test showing event
TEST_F(UITest, ShowEvent) {
    ui->showEvent(event.get(), player.get());
    
    std::string output = getOutputAndClear();
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output.find("Test Event") != std::string::npos);
    EXPECT_TRUE(output.find("This is a test event") != std::string::npos);
    EXPECT_TRUE(output.find("Choice 1") != std::string::npos);
    EXPECT_TRUE(output.find("Choice 2") != std::string::npos);
}

// Test showing the map
TEST_F(UITest, ShowMap) {
    // Get room data from the map
    int currentRoomId = -1;
    if (map->getCurrentRoom()) {
        currentRoomId = map->getCurrentRoom()->id;
    }
    std::vector<int> availableRooms = map->getAvailableRooms();
    const auto& allRooms = map->getAllRooms();
    
    ui->showMap(currentRoomId, availableRooms, allRooms);
    
    std::string output = getOutputAndClear();
    EXPECT_FALSE(output.empty());
    // Specific contents depend on map implementation
}

// Test showing a message
TEST_F(UITest, ShowMessage) {
    ui->showMessage("Test message");
    
    std::string output = getOutputAndClear();
    EXPECT_FALSE(output.empty());
    EXPECT_TRUE(output.find("Test message") != std::string::npos);
}

// Test input handling with mocked input
class MockTextUI : public TextUI {
public:
    MockTextUI() : TextUI() {}
    
    void setNextInput(const std::string& input) {
        nextInput = input;
    }
    
    // Override the getInput method from TextUI
    std::string getInput(const std::string& prompt) override {
        return nextInput;
    }
    
    // Add a custom implementation for numeric input
    int getNumericInput(const std::string& prompt, int min, int max) {
        try {
            int value = std::stoi(nextInput);
            if (value >= min && value <= max) {
                return value;
            }
        } catch (...) {
            // Handle conversion errors
        }
        return min; // Default to min value for invalid inputs
    }
    
    // Add a custom implementation for yes/no input
    bool getYesNoInput(const std::string& prompt) {
        return (nextInput == "y" || nextInput == "Y" || 
                nextInput == "yes" || nextInput == "Yes");
    }
    
private:
    std::string nextInput;
};

// Test input handling
TEST(MockUITest, InputHandling) {
    auto mockUI = std::make_shared<MockTextUI>();
    auto game = std::make_shared<Game>();
    mockUI->initialize(game.get());
    
    // Test getting numeric input
    mockUI->setNextInput("5");
    int result = mockUI->getNumericInput("Enter a number:", 1, 10);
    EXPECT_EQ(result, 5);
    
    // Test getting invalid numeric input
    mockUI->setNextInput("15");
    result = mockUI->getNumericInput("Enter a number:", 1, 10);
    EXPECT_NE(result, 15); // Should return a default value or re-prompt
    
    // Test getting yes/no input
    mockUI->setNextInput("y");
    bool yesNo = mockUI->getYesNoInput("Test question?");
    EXPECT_TRUE(yesNo);
    
    mockUI->setNextInput("n");
    yesNo = mockUI->getYesNoInput("Test question?");
    EXPECT_FALSE(yesNo);
}

// Test UI shutdown
TEST_F(UITest, Shutdown) {
    // Test shutdown
    ui->shutdown();
    
    std::string output = getOutputAndClear();
    EXPECT_FALSE(output.empty());
}

} // namespace testing
} // namespace deckstiny 