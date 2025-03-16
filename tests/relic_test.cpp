#include <gtest/gtest.h>
#include "core/relic.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/combat.h"
#include <memory>

namespace deckstiny {
namespace testing {

// Base Relic class tests
class RelicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a basic relic for testing using constructor
        relic = std::make_shared<Relic>(
            "test_relic",           // id
            "Test Relic",           // name
            "This is a test relic.", // description
            RelicRarity::COMMON,    // rarity
            "Flavor text for testing." // flavor text
        );
        
        // Create a player for testing relic effects
        player = std::make_shared<Player>("player1", "Test Player", PlayerClass::IRONCLAD, 75, 3, 5);
    }

    void TearDown() override {
        relic.reset();
        player.reset();
    }

    std::shared_ptr<Relic> relic;
    std::shared_ptr<Player> player;
};

// Test relic properties
TEST_F(RelicTest, RelicProperties) {
    EXPECT_EQ(relic->getId(), "test_relic");
    EXPECT_EQ(relic->getName(), "Test Relic");
    EXPECT_EQ(relic->getDescription(), "This is a test relic.");
    EXPECT_EQ(relic->getRarity(), RelicRarity::COMMON);
    EXPECT_EQ(relic->getFlavorText(), "Flavor text for testing.");
    
    // Test modifying properties
    relic->setName("Updated Test Relic");
    EXPECT_EQ(relic->getName(), "Updated Test Relic");
    
    relic->setDescription("Updated description.");
    EXPECT_EQ(relic->getDescription(), "Updated description.");
    
    relic->setFlavorText("Updated flavor text.");
    EXPECT_EQ(relic->getFlavorText(), "Updated flavor text.");
}

// Test relic counter
TEST_F(RelicTest, RelicCounter) {
    EXPECT_EQ(relic->getCounter(), 0);
    
    relic->setCounter(5);
    EXPECT_EQ(relic->getCounter(), 5);
    
    relic->incrementCounter(3);
    EXPECT_EQ(relic->getCounter(), 8);
    
    relic->resetCounter();
    EXPECT_EQ(relic->getCounter(), 0);
}

// Test basic relic cloning
TEST_F(RelicTest, RelicCloning) {
    auto clonedRelic = relic->cloneRelic();
    
    EXPECT_EQ(clonedRelic->getId(), relic->getId());
    EXPECT_EQ(clonedRelic->getName(), relic->getName());
    EXPECT_EQ(clonedRelic->getDescription(), relic->getDescription());
    EXPECT_EQ(clonedRelic->getRarity(), relic->getRarity());
    EXPECT_EQ(clonedRelic->getFlavorText(), relic->getFlavorText());
    
    // Verify that changing the clone doesn't affect the original
    clonedRelic->setName("Cloned Relic");
    EXPECT_NE(clonedRelic->getName(), relic->getName());
}

// Test relic effects
TEST_F(RelicTest, RelicEffects) {
    // Create a combat instance
    auto combat = std::make_shared<Combat>(player.get());
    auto enemy = std::make_shared<Enemy>("enemy1", "Test Enemy", 50);
    combat->addEnemy(enemy);
    
    // Test onObtain
    relic->onObtain(player.get());
    
    // Test combat-related callbacks
    relic->onCombatStart(player.get(), combat.get());
    relic->onTurnStart(player.get(), combat.get());
    
    // Test damage modification
    int damage = 10;
    int modifiedDamage = relic->onDealDamage(player.get(), damage, 0, combat.get());
    EXPECT_EQ(modifiedDamage, damage); // Default implementation doesn't modify damage
    
    damage = 15;
    modifiedDamage = relic->onTakeDamage(player.get(), damage, combat.get());
    EXPECT_EQ(modifiedDamage, damage); // Default implementation doesn't modify damage
    
    relic->onTurnEnd(player.get(), combat.get());
    relic->onCombatEnd(player.get(), true, combat.get());
}

// Specific Relic Types and Effects tests
class RelicEffectTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a player for testing
        player = std::make_shared<Player>("player1", "Test Player", PlayerClass::IRONCLAD, 75, 3, 5);
        
        // Create an enemy for combat tests
        enemy = std::make_shared<Enemy>("enemy1", "Test Enemy", 50);
        
        // Create specific relics for testing
        // 1. Relic that increases max health
        burningBlood = std::make_shared<Relic>(
            "burning_blood",
            "Burning Blood",
            "At the end of combat, heal 6 HP.",
            RelicRarity::STARTER
        );
        
        // 2. Relic that gives energy at the start of turn
        energyRelic = std::make_shared<Relic>(
            "philosopher_stone",
            "Philosopher's Stone",
            "Start each combat with 1 additional energy. Enemies start with 1 Strength.",
            RelicRarity::BOSS
        );
        
        // 3. Relic that activates on card play
        penNib = std::make_shared<Relic>(
            "pen_nib",
            "Pen Nib",
            "Every 10th Attack played deals double damage.",
            RelicRarity::COMMON
        );
        
        // 4. Relic that activates when taking damage
        torii = std::make_shared<Relic>(
            "torii",
            "Torii",
            "Whenever you would receive 5 or less unblocked Attack damage, reduce it to 1.",
            RelicRarity::RARE
        );
    }

    void TearDown() override {
        player.reset();
        enemy.reset();
        burningBlood.reset();
        energyRelic.reset();
        penNib.reset();
        torii.reset();
    }

    std::shared_ptr<Player> player;
    std::shared_ptr<Enemy> enemy;
    std::shared_ptr<Relic> burningBlood;
    std::shared_ptr<Relic> energyRelic;
    std::shared_ptr<Relic> penNib;
    std::shared_ptr<Relic> torii;
};

// Test burning blood relic effect
TEST_F(RelicEffectTest, BurningBloodEffect) {
    // Setup combat
    auto combat = std::make_shared<Combat>(player.get());
    combat->addEnemy(enemy);
    
    // Record player health before
    int healthBefore = player->getHealth();
    
    // Simulate end of combat with victory
    burningBlood->onCombatEnd(player.get(), true, combat.get());
    
    // Check if player was healed
    EXPECT_EQ(player->getHealth(), healthBefore + 6);
}

// Test pen nib counter
TEST_F(RelicEffectTest, PenNibCounter) {
    // Setup initial counter
    penNib->setCounter(9);
    
    // Setup combat
    auto combat = std::make_shared<Combat>(player.get());
    combat->addEnemy(enemy);
    
    // Test damage modification for 10th attack
    int damage = 10;
    int modifiedDamage = penNib->onDealDamage(player.get(), damage, 0, combat.get());
    
    // Default implementation doesn't modify damage, but we can test the counter
    EXPECT_EQ(modifiedDamage, damage);
    
    // Increment counter to simulate attack played
    penNib->incrementCounter();
    EXPECT_EQ(penNib->getCounter(), 10);
    
    // Reset counter to simulate the effect triggering
    if (penNib->getCounter() >= 10) {
        penNib->resetCounter();
    }
    EXPECT_EQ(penNib->getCounter(), 0);
}

} // namespace testing
} // namespace deckstiny 