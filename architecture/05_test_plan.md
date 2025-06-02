# Test Plan

## Overview

This test plan outlines the comprehensive approach undertaken for testing the Deckstiny project, a key component of our 2nd semester C++ programming course. The testing strategy successfully employed a combination of unit tests for individual components and integration tests to verify complex interactions and core game functionality. Google Test was utilized as the testing framework, integrated with CTest for execution and management.

## Unit Tests

The focus of unit testing was on thoroughly validating the core data structures and their individual functionalities, ensuring a robust foundation for the game.

### Entity System Tests
-   Entity properties and behavior were rigorously tested through derived classes such as `Character`, `Player`, and `Enemy`, ensuring fundamental object characteristics are correct.

### Card System Tests (`CardTest`)
-   Successfully tested card creation, all defined properties (name, cost, type, description, rarity), and JSON data loading.
-   Verified card effect application logic for key card types.
-   Confirmed card targeting logic for various scenarios.
-   Ensured card cloning and upgrade mechanisms function as designed, reflecting changes in stats and behavior.

### Character System Tests (`CharacterTest`)
-   Validated character creation and accurate representation of basic properties.
-   Confirmed health and block mechanics, including damage application, healing, and block gain/loss, under various conditions.
-   Tested status effect application and their fundamental impacts on characters.

### Player-Specific Tests (`PlayerTest`)
-   Thoroughly tested deck management features, including adding cards to the deck.
-   Validated card pile operations such as drawing and discarding.
-   Ensured energy management mechanics (gain, spend) work correctly.
-   Verified relic management, including adding relics and the triggering of their basic effects.
-   Confirmed the player's role and actions within combat functions operate as expected.

### Relic System Tests (`RelicTest`, `RelicEffectTest`)
-   Tested relic properties, counter behaviors, and successful cloning.
-   Verified the specific effects of important relics like "Burning Blood" and "Pen Nib", ensuring they trigger correctly and produce the intended outcomes.

### Event System Tests (`EventTest`)
-   Validated event properties, successful cloning, and correct triggering of event effects based on player choices.
-   Confirmed the availability of choices as defined and the accurate loading of event data from JSON files.

### UI System Tests (`UITest`, `MockUITest`)
-   Successfully tested UI interaction flows using `MockUI`, ensuring game logic responds correctly to simulated UI events.
-   Verified that all necessary calls to UI methods for displaying game states (main menu, combat, map, events, shop) are made appropriately.
-   Confirmed input handling through the UI callback mechanism is robust.

## Integration Tests

Integration tests focused on the seamless interaction between different game components, ensuring the overall stability and correctness of larger game systems.

### Combat System Tests (`CombatTest`)
-   Successfully tested combat initiation with various player and enemy configurations.
-   Validated player turn execution, including playing different types of cards and correctly ending turns.
-   Ensured enemy turn execution, including intent selection and the accurate application of those intents.
-   Confirmed combat resolution for both player victory and player defeat scenarios.
-   Verified damage application mechanics, including lethal damage and the proper functioning of upgraded cards.
-   Tested key combat mechanics like status effects (e.g., Weak, Vulnerable) and their impact.

### Map System Tests (`MapTest`)
-   Thoroughly tested the new Slay the Spire-style map generation logic, ensuring diverse and playable maps.
-   Validated map structure, including node connectivity, correct y-coordinate progression for floors, and path variety.
-   Confirmed player navigation on the map functions as intended.
-   Ensured pathfinding to the boss room is always possible.
-   Verified map completion logic.

### Game Flow and Data Loading Tests (`GameTest`)
-   Successfully tested complete game initialization, including the robust loading of all game data (cards, enemies, relics, events, characters) from JSON.
-   Validated all defined game state transitions (e.g., Main Menu -> Character Select -> Map -> Combat -> Event -> Shop -> Reward -> Game Over).
-   Confirmed the player creation process, including starting deck and relic setup.
-   Ensured map generation is correctly integrated within the game flow.
-   Verified combat initialization from map nodes.
-   Tested high-level input processing for all different game states, ensuring correct responses.

## Test Implementation Strategy

1.  **Testing Framework**: Google Test (gtest/gmock) was successfully integrated with CTest for automated test execution and reporting.
2.  **Testing Approach**:
    *   Comprehensive unit tests were written for each major component, establishing individual correctness.
    *   Extensive integration tests were developed to verify the interplay between these components.
    *   `MockUI` was effectively utilized for testing game logic interacting with the UI, enabling fully automated tests without requiring manual UI rendering or user input.
3.  **Test Organization**: Tests were logically grouped into suites based on the component or functionality under test (e.g., `CardTest`, `CombatTest`, `GameTest`), facilitating clear organization and targeted execution.

## Example Test Case Format (Illustrative)

```cpp
// Example from CombatTest demonstrating damage verification
TEST_F(CombatTest, PlayerDealsDamageWithStrike) {
    combat->start(); // Initialize combat and player/enemy states
    ASSERT_TRUE(player->getHand().size() > 0) << "Player's hand should not be empty.";

    int strikeIdx = findCardInHand("strike"); // Custom helper to locate a specific card
    ASSERT_NE(strikeIdx, -1) << "Strike card must be present in hand for this test.";

    Card* cardToPlay = player->getHand()[strikeIdx].get();
    int enemyInitialHealth = enemy->getHealth();
    int expectedDamage = 6; // Known damage for a standard Strike card

    bool playedSuccessfully = combat->playCard(strikeIdx, 0); // Play Strike on the first enemy
    ASSERT_TRUE(playedSuccessfully) << "Playing Strike card failed unexpectedly.";

    EXPECT_EQ(enemy->getHealth(), enemyInitialHealth - expectedDamage) << "Enemy health not reduced correctly after Strike.";
}
```

## Testing Progress Tracking

This table summarizes the final status of tests written and their execution results, based on the latest CTest run.

| Test Suite / Component | Tests Written (Unit) | Tests Passing (Unit) | Tests Written (Integ.) | Tests Passing (Integ.) | Notes                                                        |
|------------------------|----------------------|----------------------|------------------------|------------------------|--------------------------------------------------------------|
| CharacterTest          | 3                    | 3                    | -                      | -                      | Core character mechanics fully verified.                     |
| PlayerTest             | 5                    | 5                    | -                      | -                      | Player-specific actions and systems thoroughly tested.       |
| CardTest               | 4                    | 4                    | -                      | -                      | Card properties, cloning, and upgrades validated.            |
| RelicTest/EffectTest   | 6                    | 6                    | -                      | -                      | Relic attributes and specific effects confirmed.             |
| EventTest              | 5                    | 5                    | -                      | -                      | Event structure, choices, and loading mechanisms tested.     |
| UITest/MockUITest      | 11                   | 11                   | -                      | -                      | UI logic and interactions robustly tested via mocking.       |
| CombatTest             | -                    | -                    | 10                     | 10                     | Combat lifecycle, turns, and damage mechanics validated.     |
| MapTest                | -                    | -                    | 6                      | 6                      | New map generation, structure, and navigation verified.      |
| GameTest               | -                    | -                    | 10                     | 10                     | Game states, data loading, and input handling fully tested.  |
| **TOTALS**             | **34**               | **34**               | **26**                 | **26**                 | **Overall: 61 distinct tests executed, 100% passing.** |
