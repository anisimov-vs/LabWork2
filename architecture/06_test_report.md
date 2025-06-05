# Test Report: Deckstiny Project

## 1. Introduction

This document presents the final test report for the Deckstiny project, developed as a requirement for the 2nd semester C++ programming course. The primary objective of the testing phase was to rigorously verify the functional correctness of all core game mechanics, ensure the stability of data handling processes, and confirm the smooth operation of the overall game flow. This was achieved through a structured approach involving unit testing of individual software components and integration testing of larger, interconnected game systems. The successful completion of this testing phase underscores the robustness and quality of the developed game.

## 2. Test Environment

*   **Testing Framework:** Google Test (gtest and gmock libraries)
*   **Build System & Test Runner:** CMake, integrated with CTest for automated test execution
*   **Development & Primary Testing Platform:** macOS
*   **Programming Language:** C++17

## 3. Test Execution Summary

*   **Date of Final Test Cycle:** June 1, 2025
*   **Total Distinct Test Cases Executed:** 61
*   **Total Test Cases Passed:** 61
*   **Total Test Cases Failed:** 0
*   **Overall Pass Rate:** 100%
*   **Total Test Execution Time:** 3.94 seconds

The comprehensive test suite, comprising 61 distinct test cases organized into 9 test suites, was executed using CTest, achieving a 100% pass rate. All tests were executed efficiently with an average execution time of approximately 0.06 seconds per test. This outcome demonstrates the stability and correctness of the implemented features.

## 4. Summary of Tested Components and Functionalities

The testing efforts were extensive, covering all critical aspects of the Deckstiny game. All tests for the following components and functionalities passed successfully:

*   **Core Entity & Character Systems (`CharacterTest`, `PlayerTest`):** Fundamental mechanics such as health and block management, status effect application, player-specific energy systems, deck interactions, and relic management were thoroughly validated.
*   **Card System (`CardTest`):** All aspects of card functionality, including property definitions, correct application of effects as per JSON data, targeting logic, cloning, and the upgrade system (both visual and mechanical changes) were confirmed to be working as intended.
*   **Relic System (`RelicTest`, `RelicEffectTest`):** Relic properties, including counters and cloning, were verified. Specific, crucial relic effects, such as "Burning Blood" and "Pen Nib," were tested to ensure they trigger correctly and provide the expected benefits.
*   **Event System (`EventTest`):** The structure and behavior of in-game events, including the presentation of choices, triggering of associated effects, and accurate loading from JSON data, were successfully tested.
*   **Combat System (`CombatTest`):** The entire combat loop, from initiation through player and enemy turns to resolution (victory/defeat), was extensively tested. This included validation of damage calculations (considering effects like Weak and Vulnerable), status effect applications, enemy AI intent processing (including The Collector's summon), and card playability rules.
*   **Map System (`MapTest`):** The newly implemented Slay the Spire-style map generation was a key focus. Tests confirmed the structural integrity of generated maps, reliable pathfinding to the boss, correct node connectivity, floor progression, and appropriate diversity in initial path choices.
*   **Game Logic & Flow (`GameTest`):** Overall game integrity was verified, including successful initialization, robust loading of all game data (cards, enemies, relics, events, character templates) from JSON files using a corrected pathing system, smooth game state transitions, and accurate player character creation. High-level input processing across various game states was also confirmed.
*   **UI System (`UITest`, `MockUITest`):** Game logic interactions with the UI were effectively tested using the `MockUI` framework. This ensured that the correct UI methods are called for displaying different game states and that the game responds appropriately to simulated user inputs, all without requiring manual UI interaction during tests.

## 5. Key Enhancements and Fixes Implemented During Testing

The iterative testing process was instrumental in identifying areas for improvement and resolving several key issues, significantly enhancing the game's stability and functionality:

*   **Map Generation Overhaul:**
    *   The map generation system was completely redesigned to emulate the branching, multi-path structure found in "Slay the Spire." This involved implementing new algorithms for path weaving, node connectivity, and ensuring all paths lead to the boss.
    *   Specific rules were implemented to diversify room types and prevent undesirable sequences, such as a starting MONSTER room leading to a single SHOP or EVENT, ensuring more meaningful player choices from the outset.
*   **Robust Data File Loading:**
    *   Initial issues with inconsistent loading of JSON data files (cards, enemies, relics, etc.), particularly in the test environment due to relative path problems, were fully resolved. A centralized and robust path prefix utility (`get_data_path_prefix`) was implemented and integrated across all data loading functions.
*   **Enhanced UI Testability:**
    *   The `TextUI` was refactored to include a `testingMode`, which, when activated by the test environment, successfully suppresses console output and blocking input calls. This, coupled with the `MockUI` framework, enabled comprehensive and automated testing of game logic that interacts with the user interface.
*   **Critical Gameplay Logic Bug Fixes:**
    *   **Card Effect Application:** A significant bug preventing enemies from taking damage from cards like "Strike" and "Bash" was identified and fixed. This involved correcting how `Card::onPlay` sources damage values, ensuring it uses data from the effect's JSON definition rather than unpopulated member variables.
    *   **Card Playability:** An issue where "Bash" could not be played despite sufficient energy was traced to `Card::onPlay` returning `false` if a secondary effect (like applying Vulnerable) couldn't be applied to an already defeated target. This logic was made more robust.
    *   **Card Upgrades:** Functionality for card upgrades was fully implemented. Cards now correctly load upgrade details from JSON, the `Card::upgrade()` method applies these changes (name, description, stats), and `Card::onPlay` uses the upgraded values. An interactive UI for choosing which card to upgrade during events was also implemented.
    *   **Event Effect Processing:**
        *   Corrected mismatches between event JSON effect type strings (e.g., "INCREASE_MAX_HEALTH", "GAIN_HEALTH") and the code expecting different strings (e.g., "MAX_HP", "HP").
        *   Resolved duplicated event result messages by ensuring the primary descriptive text comes from the event's JSON definition, while the code handles the mechanical application of effects.
        *   Ensured relic award messages correctly state the name of the relic obtained.
    *   **Shop Functionality:** The shop system was significantly improved. A `Game::startShop()` method was implemented to properly populate the shop inventory with cloned items and assign prices. `Game::handleShopInput` was made more robust to parse "C1", "R1" style inputs and handle basic purchase logic.
    *   **Boss Mechanics:** "The Collector" boss's "summon" intent, which was previously non-functional, was implemented in `Enemy::takeTurn` to correctly summon minions.
    *   **Status Effect Implementation:** The "Weak" status effect now correctly reduces outgoing damage by 25% for both players and enemies. "Vulnerable" status effect logic was centralized in `Character::takeDamage` to correctly increase incoming damage.
    *   **Rest Site Functionality:** Healing at rest sites was confirmed to be working correctly.
    *   **HP Discrepancies:** An issue where player HP appeared to increase between fights was traced to the "Burning Blood" relic functioning as intended (healing 6HP after combat victory).
*   **Build Stability and Code Cleanup:**
    *   Numerous minor compilation warnings and linker issues (e.g., related to duplicate library linking, unused variables) were addressed.
    *   Unnecessary debug console output from the main game executable was removed (e.g., `TextUI::isTestingMode` checks, `Game::processInput` traces), providing a cleaner user experience.

## 6. Test Coverage Assessment

The current test suite provides comprehensive coverage of the Deckstiny project's core features and critical systems. Unit tests validate the fundamental building blocks of the game, including entities, cards, characters, players, relics, and events. Integration tests ensure that these components work together harmoniously in more complex scenarios such as combat encounters, map navigation, and overall game state transitions.

Key areas with strong test coverage include:
*   **Data Integrity:** Loading of all game entities and items from JSON files.
*   **Core Mechanics:** Health, block, energy, status effects, card play.
*   **Game Flow:** Initialization, state changes, player progression through map nodes.
*   **Map Generation:** Creation of valid, navigable, and diverse Slay the Spire-style maps.
*   **Combat Logic:** Turn structure, damage dealing, enemy AI behavior, and combat resolution.

The robust suite of 61 passing tests forms a solid foundation, ensuring that the main gameplay loop and critical systems perform as expected and providing high confidence in the quality and stability of the current build. This allows for future development and feature additions to be built upon a well-verified codebase.

## 7. Conclusion

The testing phase for the Deckstiny project has been successfully completed, with all 61 implemented unit and integration tests passing. This rigorous process has not only verified the existing functionality but also led to the identification and resolution of numerous significant bugs and areas for improvement, particularly in map generation, data handling, and core gameplay mechanics. The game now stands as a stable, functional, and well-tested application, demonstrating a solid understanding and application of C++ programming principles and software testing practices.