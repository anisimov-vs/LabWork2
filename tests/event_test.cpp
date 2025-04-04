#include <gtest/gtest.h>
#include "core/event.h"
#include "core/player.h"
#include "core/card.h"
#include "core/relic.h"
#include "core/game.h"
#include <memory>

namespace deckstiny {
namespace testing {

// Base Event class tests
class EventTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a player for testing events
        player = std::make_shared<Player>("player1", "Test Player", PlayerClass::IRONCLAD, 75, 3, 5);
        
        // Create a basic event for testing using JSON
        event = std::make_shared<Event>();
        
        // Add choices to the event by loading from JSON
        nlohmann::json eventJson = {
            {"id", "test_event"},
            {"name", "Test Event"},
            {"description", "This is a test event with multiple choices."},
            {"choices", nlohmann::json::array({
                {
                    {"text", "Gain 20 gold"},
                    {"resultText", "You gained 20 gold."},
                    {"effects", nlohmann::json::array({
                        {{"effect", "GAIN_GOLD"}, {"value", 20}}
                    })}
                },
                {
                    {"text", "Lose 5 health, gain a relic"},
                    {"resultText", "You lost 5 health but gained a relic."},
                    {"requiresHealth", 5},
                    {"effects", nlohmann::json::array({
                        {{"effect", "LOSE_HEALTH"}, {"value", 5}},
                        {{"effect", "GAIN_RELIC"}, {"value", "test_relic"}}
                    })}
                },
                {
                    {"text", "Leave"},
                    {"resultText", "You decided to leave."},
                    {"effects", nlohmann::json::array()}
                }
            })}
        };
        
        // Load the event from JSON
        event->loadFromJson(eventJson);
        
        // Create a game instance for testing event processing
        game = std::make_shared<Game>();
    }

    void TearDown() override {
        player.reset();
        event.reset();
        game.reset();
    }

    std::shared_ptr<Player> player;
    std::shared_ptr<Event> event;
    std::shared_ptr<Game> game;
};

// Test event properties
TEST_F(EventTest, EventProperties) {
    EXPECT_EQ(event->getId(), "test_event");
    EXPECT_EQ(event->getName(), "Test Event");
    EXPECT_EQ(event->getDescription(), "This is a test event with multiple choices.");
    
    // Test number of choices
    EXPECT_EQ(event->getAllChoices().size(), 3);
    
    // Test choice texts
    EXPECT_EQ(event->getAllChoices()[0].text, "Gain 20 gold");
    EXPECT_EQ(event->getAllChoices()[1].text, "Lose 5 health, gain a relic");
    EXPECT_EQ(event->getAllChoices()[2].text, "Leave");
}

// Test event cloning
TEST_F(EventTest, EventCloning) {
    auto clonedEvent = std::unique_ptr<Event>(static_cast<Event*>(event->clone().release()));
    
    EXPECT_EQ(clonedEvent->getId(), event->getId());
    EXPECT_EQ(clonedEvent->getName(), event->getName());
    EXPECT_EQ(clonedEvent->getDescription(), event->getDescription());
    EXPECT_EQ(clonedEvent->getAllChoices().size(), event->getAllChoices().size());
}

// Test event effects
TEST_F(EventTest, EventEffects) {
    // This test would normally use the Game class to process choices
    // Since we can't fully test the effects without a Game instance,
    // we'll just verify the structure of the effects
    
    const auto& choices = event->getAllChoices();
    
    // First choice: Gain 20 gold
    EXPECT_EQ(choices[0].effects.size(), 1);
    EXPECT_EQ(choices[0].effects[0].type, "GAIN_GOLD");
    EXPECT_EQ(choices[0].effects[0].value, 20);
    
    // Second choice: Lose 5 health, gain a relic
    EXPECT_EQ(choices[1].effects.size(), 2);
    EXPECT_EQ(choices[1].effects[0].type, "LOSE_HEALTH");
    EXPECT_EQ(choices[1].effects[0].value, 5);
    EXPECT_EQ(choices[1].effects[1].type, "GAIN_RELIC");
    EXPECT_EQ(choices[1].effects[1].target, "test_relic");
    
    // Third choice: Leave (no effects)
    EXPECT_EQ(choices[2].effects.size(), 0);
}

// Test event available choices
TEST_F(EventTest, AvailableChoices) {
    // Test getting available choices
    const auto& availableChoices = event->getAvailableChoices(player.get());
    
    // All choices should be available initially
    EXPECT_EQ(availableChoices.size(), 3);
    
    // Test choice requirements
    EXPECT_EQ(availableChoices[0].goldCost, 0);
    EXPECT_EQ(availableChoices[1].healthCost, 5);
}

// Test event JSON loading
TEST_F(EventTest, EventJsonLoading) {
    // Create a new event from JSON
    nlohmann::json eventJson = {
        {"id", "json_test_event"},
        {"name", "JSON Test Event"},
        {"description", "This event was loaded from JSON."},
        {"choices", nlohmann::json::array({
            {
                {"text", "Choice 1"},
                {"resultText", "Result 1"},
                {"effects", nlohmann::json::array({
                    {{"effect", "GAIN_GOLD"}, {"value", 10}}
                })}
            },
            {
                {"text", "Choice 2"},
                {"resultText", "Result 2"},
                {"effects", nlohmann::json::array()}
            }
        })}
    };
    
    auto newEvent = std::make_shared<Event>();
    EXPECT_TRUE(newEvent->loadFromJson(eventJson));
    
    // Verify the event was loaded correctly
    EXPECT_EQ(newEvent->getId(), "json_test_event");
    EXPECT_EQ(newEvent->getName(), "JSON Test Event");
    EXPECT_EQ(newEvent->getDescription(), "This event was loaded from JSON.");
    
    // Verify choices were loaded
    EXPECT_EQ(newEvent->getAllChoices().size(), 2);
    EXPECT_EQ(newEvent->getAllChoices()[0].text, "Choice 1");
    EXPECT_EQ(newEvent->getAllChoices()[1].text, "Choice 2");
    
    // Verify effects were loaded
    EXPECT_EQ(newEvent->getAllChoices()[0].effects.size(), 1);
    EXPECT_EQ(newEvent->getAllChoices()[0].effects[0].type, "GAIN_GOLD");
    EXPECT_EQ(newEvent->getAllChoices()[0].effects[0].value, 10);
}

} // namespace testing
} // namespace deckstiny 