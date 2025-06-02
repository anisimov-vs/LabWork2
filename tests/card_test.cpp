#include <gtest/gtest.h>
#include "core/card.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/combat.h"
#include <memory>

namespace deckstiny {
namespace testing {

// Base Card class tests
class CardTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a card for testing using constructor instead of setters
        card = std::make_shared<Card>(
            "test_card",           // id
            "Test Card",           // name
            "This is a test card.", // description
            CardType::ATTACK,      // type
            CardRarity::COMMON,    // rarity
            CardTarget::NONE,      // target
            1,                     // cost
            true                   // upgradable
        );
        
        // Create a player and enemy for testing card effects
        player = std::make_shared<Player>("ironclad", "Test Player", 75, 3, 5);
        enemy = std::make_shared<Enemy>("enemy1", "Test Enemy", 50);
    }

    void TearDown() override {
        card.reset();
        player.reset();
        enemy.reset();
    }

    std::shared_ptr<Card> card;
    std::shared_ptr<Player> player;
    std::shared_ptr<Enemy> enemy;
};

// Test card properties
TEST_F(CardTest, CardProperties) {
    EXPECT_EQ(card->getId(), "test_card");
    EXPECT_EQ(card->getName(), "Test Card");
    EXPECT_EQ(card->getDescription(), "This is a test card.");
    EXPECT_EQ(card->getType(), CardType::ATTACK);
    EXPECT_EQ(card->getCost(), 1);
    EXPECT_EQ(card->getRarity(), CardRarity::COMMON);
    
    // Test modifying properties
    card->setName("Updated Test Card");
    EXPECT_EQ(card->getName(), "Updated Test Card");
    
    card->setCost(2);
    EXPECT_EQ(card->getCost(), 2);
    
    card->setDescription("Updated description");
    EXPECT_EQ(card->getDescription(), "Updated description");
}

// Test card upgrade
TEST_F(CardTest, CardUpgrade) {
    EXPECT_TRUE(card->isUpgradable());
    EXPECT_FALSE(card->isUpgraded());
    
    // Upgrade the card
    EXPECT_TRUE(card->upgrade());
    EXPECT_TRUE(card->isUpgraded());
    
    // Cost should be reduced by 1 (default upgrade behavior)
    EXPECT_EQ(card->getCost(), 0);
    
    // Cannot upgrade again
    EXPECT_FALSE(card->upgrade());
}

// Test card cloning
TEST_F(CardTest, CardCloning) {
    auto clonedCard = card->cloneCard();
    
    EXPECT_EQ(clonedCard->getId(), card->getId());
    EXPECT_EQ(clonedCard->getName(), card->getName());
    EXPECT_EQ(clonedCard->getDescription(), card->getDescription());
    EXPECT_EQ(clonedCard->getType(), card->getType());
    EXPECT_EQ(clonedCard->getRarity(), card->getRarity());
    EXPECT_EQ(clonedCard->getCost(), card->getCost());
    EXPECT_EQ(clonedCard->isUpgradable(), card->isUpgradable());
    EXPECT_EQ(clonedCard->isUpgraded(), card->isUpgraded());
}

// Test card target validation
TEST_F(CardTest, CardTargeting) {
    // Create a combat instance
    auto combat = std::make_shared<Combat>(player.get());
    combat->addEnemy(enemy);
    
    // Start combat for player and combat object
    player->beginCombat(); // Initialize player for combat (e.g., set current energy)
    combat->start();       // Initialize combat state (e.g., enemy intents)
    
    // Test card with no target
    auto noTargetCard = std::make_shared<Card>(
        "no_target", "No Target Card", "No target needed", 
        CardType::SKILL, CardRarity::COMMON, CardTarget::NONE, 1, true
    );
    
    EXPECT_TRUE(noTargetCard->canPlay(player.get(), -1, combat.get()));
    
    // Test card with single enemy target
    auto singleTargetCard = std::make_shared<Card>(
        "single_target", "Single Target Card", "Needs enemy target", 
        CardType::ATTACK, CardRarity::COMMON, CardTarget::SINGLE_ENEMY, 1, true
    );
    
    EXPECT_TRUE(singleTargetCard->canPlay(player.get(), 0, combat.get())); // Valid enemy index
    EXPECT_FALSE(singleTargetCard->canPlay(player.get(), 1, combat.get())); // Invalid enemy index
    
    // Test card with self target
    auto selfTargetCard = std::make_shared<Card>(
        "self_target", "Self Target Card", "Targets self", 
        CardType::SKILL, CardRarity::COMMON, CardTarget::SELF, 1, true
    );
    
    EXPECT_TRUE(selfTargetCard->canPlay(player.get(), 0, combat.get())); // Self
    EXPECT_TRUE(selfTargetCard->canPlay(player.get(), -1, combat.get())); // No target (defaults to self)
}

} // namespace testing
} // namespace deckstiny 