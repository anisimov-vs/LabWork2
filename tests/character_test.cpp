// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include <gtest/gtest.h>
#include "core/character.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/card.h"
#include "core/relic.h"
#include <memory>

namespace deckstiny {
namespace testing {

// Base Character class tests
class CharacterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a concrete Character instance for testing (using Player since Character is abstract)
        character = std::make_unique<Player>("ironclad", "Test Character", 50, 3, 5);
    }

    void TearDown() override {
        character.reset();
    }

    std::unique_ptr<Player> character;
};

// Test health management
TEST_F(CharacterTest, HealthManagement) {
    // Verify initial values
    EXPECT_EQ(character->getHealth(), 50);
    EXPECT_EQ(character->getMaxHealth(), 50);
    EXPECT_TRUE(character->isAlive());
    
    // Test taking damage
    character->takeDamage(10);
    EXPECT_EQ(character->getHealth(), 40);
    EXPECT_TRUE(character->isAlive());
    
    // Test healing
    character->heal(5);
    EXPECT_EQ(character->getHealth(), 45);
    
    // Test healing beyond max health
    character->heal(20);
    EXPECT_EQ(character->getHealth(), 50); // Should be capped at max health
    
    // Test fatal damage
    character->takeDamage(60);
    EXPECT_EQ(character->getHealth(), 0);
    EXPECT_FALSE(character->isAlive());
    
    // Test healing when dead (should have no effect)
    character->heal(10);
    EXPECT_EQ(character->getHealth(), 0);
    EXPECT_FALSE(character->isAlive());
    
    // Test increasing max health
    character->setHealth(1); // Revive for testing
    character->increaseMaxHealth(10);
    EXPECT_EQ(character->getMaxHealth(), 60);
    EXPECT_EQ(character->getHealth(), 1); // Health doesn't increase with max health
    
    // Test setting max health lower
    character->setHealth(60);
    character->setMaxHealth(40);
    EXPECT_EQ(character->getMaxHealth(), 40);
    EXPECT_EQ(character->getHealth(), 40); // Health reduced to new max
}

// Test block management
TEST_F(CharacterTest, BlockManagement) {
    // Verify initial block
    EXPECT_EQ(character->getBlock(), 0);
    
    // Test adding block
    character->addBlock(10);
    EXPECT_EQ(character->getBlock(), 10);
    
    // Test damage mitigation with block
    int damageTaken = character->takeDamage(5);
    EXPECT_EQ(damageTaken, 5);
    EXPECT_EQ(character->getBlock(), 5); // Block reduced
    EXPECT_EQ(character->getHealth(), 50); // Health unchanged
    
    // Test damage exceeding block
    damageTaken = character->takeDamage(10);
    EXPECT_EQ(damageTaken, 10);
    EXPECT_EQ(character->getBlock(), 0); // Block depleted
    EXPECT_EQ(character->getHealth(), 45); // Health reduced by remaining damage
    
    // Test resetting block
    character->addBlock(15);
    EXPECT_EQ(character->getBlock(), 15);
    character->resetBlock();
    EXPECT_EQ(character->getBlock(), 0);
}

// Test status effects
TEST_F(CharacterTest, StatusEffects) {
    // Verify no initial status effects
    EXPECT_EQ(character->getStatusEffects().size(), 0);
    
    // Test adding a status effect
    character->addStatusEffect("Strength", 2);
    auto effects = character->getStatusEffects();
    EXPECT_EQ(effects.size(), 1);
    EXPECT_EQ(effects.at("Strength"), 2);
    
    // Test modifying an existing status effect
    character->addStatusEffect("Strength", 1);
    effects = character->getStatusEffects();
    EXPECT_EQ(effects.at("Strength"), 3);
    
    // Test adding a different status effect
    character->addStatusEffect("Dexterity", 2);
    effects = character->getStatusEffects();
    EXPECT_EQ(effects.size(), 2);
    EXPECT_EQ(effects.at("Dexterity"), 2);
    
    // Test checking for status effect
    EXPECT_TRUE(character->hasStatusEffect("Strength"));
    EXPECT_TRUE(character->hasStatusEffect("Dexterity"));
    EXPECT_FALSE(character->hasStatusEffect("Weak"));
    
    // Test getting status effect values
    EXPECT_EQ(character->getStatusEffect("Strength"), 3);
    EXPECT_EQ(character->getStatusEffect("Dexterity"), 2);
    EXPECT_EQ(character->getStatusEffect("Weak"), 0); // Not present
}

// Player-specific tests
class PlayerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a player for testing
        player = std::make_unique<Player>("ironclad", "Test Player", 75, 3, 5);
        
        // Create some cards for testing
        strike = std::make_shared<Card>();
        strike->setName("Strike");
        strike->setDescription("Deal 6 damage.");
        strike->setCost(1);
        
        defend = std::make_shared<Card>();
        defend->setName("Defend");
        defend->setDescription("Gain 5 Block.");
        defend->setCost(1);
        
        // Create a relic for testing
        relic = std::make_shared<Relic>();
        relic->setName("Burning Blood");
        relic->setDescription("At the end of combat, heal 6 HP.");
    }

    void TearDown() override {
        player.reset();
        strike.reset();
        defend.reset();
        relic.reset();
    }

    std::unique_ptr<Player> player;
    std::shared_ptr<Card> strike;
    std::shared_ptr<Card> defend;
    std::shared_ptr<Relic> relic;
};

// Test deck management
TEST_F(PlayerTest, CardManagement) {
    // Test adding cards
    player->addCardToDeck(strike);
    EXPECT_EQ(player->getDrawPile().size(), 1);
    
    player->addCardToDeck(defend);
    EXPECT_EQ(player->getDrawPile().size(), 2);
    
    // Verify the cards in the deck
    auto drawPile = player->getDrawPile();
    EXPECT_EQ(drawPile[0]->getName(), "Strike");
    EXPECT_EQ(drawPile[1]->getName(), "Defend");
}

// Test card pile management
TEST_F(PlayerTest, CardPileManagement) {
    // Add cards to deck
    player->addCardToDeck(strike);
    player->addCardToDeck(defend);
    
    // Test shuffling draw pile
    player->shuffleDrawPile();
    EXPECT_EQ(player->getDrawPile().size(), 2);
    
    // Test drawing cards
    player->drawCards(1);
    EXPECT_EQ(player->getHand().size(), 1);
    EXPECT_EQ(player->getDrawPile().size(), 1);
    
    // Test drawing more cards than in draw pile
    player->drawCards(2);
    EXPECT_EQ(player->getHand().size(), 2);
    EXPECT_EQ(player->getDrawPile().size(), 0);
    
    // Test discarding hand
    player->discardHand();
    EXPECT_EQ(player->getHand().size(), 0);
    EXPECT_EQ(player->getDiscardPile().size(), 2);
    
    // Test reshuffling discard into draw
    player->shuffleDiscardIntoDraw();
    EXPECT_EQ(player->getDiscardPile().size(), 0);
    EXPECT_EQ(player->getDrawPile().size(), 2);
}

// Test energy management
TEST_F(PlayerTest, EnergyManagement) {
    // Verify initial energy
    EXPECT_EQ(player->getEnergy(), 0);
    
    // Test setting energy
    player->setEnergy(2);
    EXPECT_EQ(player->getEnergy(), 2);
    
    // Test using energy
    EXPECT_TRUE(player->useEnergy(1));
    EXPECT_EQ(player->getEnergy(), 1);
    
    // Test trying to use more energy than available
    EXPECT_FALSE(player->useEnergy(2));
    EXPECT_EQ(player->getEnergy(), 1); // Energy shouldn't change if use fails
    
    // Test starting a turn
    player->startTurn();
    EXPECT_EQ(player->getEnergy(), 3); // Should be base energy (3 for this test player)
}

// Test relic management
TEST_F(PlayerTest, RelicManagement) {
    // Verify no initial relics
    EXPECT_EQ(player->getRelics().size(), 0);
    
    // Test adding a relic
    player->addRelic(relic);
    EXPECT_EQ(player->getRelics().size(), 1);
    EXPECT_EQ(player->getRelics()[0]->getName(), "Burning Blood");
}

// Test player combat functions
TEST_F(PlayerTest, CombatFunctions) {
    // Add cards to the player's draw pile
    player->addCardToDeck(strike);
    player->addCardToDeck(defend);
    
    // Simulate beginning combat and starting a turn
    player->beginCombat(); // Draws initial hand (2 cards in this case as deck only has 2)
    player->startTurn();   // Resets energy, block, draws up to initialHandSize (5). 
                           // Since draw pile is empty, hand remains at 2.
    
    // Check hand and discard pile state after starting turn, before any card plays
    EXPECT_EQ(player->getHand().size(), 2); // Player drew the 2 cards from deck
    EXPECT_EQ(player->getDiscardPile().size(), 0); // No cards discarded yet
}

} // namespace testing
} // namespace deckstiny 