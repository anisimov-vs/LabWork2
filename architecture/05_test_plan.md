# Test Plan

## Overview

This test plan outlines a simple approach to testing the Deckstiny project for a 2nd semester C++ programming course. The testing strategy focuses on basic unit tests and simple integration tests to verify the core functionality.

## Unit Tests

### Entity System Tests
- Test entity creation and basic properties
- Test JSON loading for entities
- Test entity cloning functionality

### Card System Tests
- Test card creation and properties
- Test card effect application
- Test card targeting logic

### Character System Tests
- Test character creation and properties
- Test health and block mechanics
- Test status effect application

### Player-Specific Tests
- Test deck management
- Test drawing and discarding cards
- Test relic effects

### Enemy-Specific Tests
- Test enemy AI decision making
- Test enemy intent generation

## Integration Tests

### Combat System Tests
- Test combat initiation
- Test player turn execution
- Test enemy turn execution
- Test combat resolution

### Game Flow Tests
- Test game initialization
- Test map navigation
- Test event encounters

## Test Implementation Strategy

1. **Testing Framework**: Use a simple testing framework like Google Test or Catch2
2. **Testing Approach**: Write tests for each major component
3. **Test Organization**: Group tests by component/functionality

## Example Test Case Format

```cpp
TEST_CASE("Card damage calculation") {
    Card attackCard("Attack", 1, CardType::ATTACK);
    attackCard.setDamage(10);
    
    Character target(100, 0);
    attackCard.applyEffect(&target);
    
    REQUIRE(target.getHealth() == 90);
}
```

## Testing Progress Tracking

| Component | Unit Tests Written | Unit Tests Passing | Integration Tests Written | Integration Tests Passing |
|-----------|-------------------|-------------------|--------------------------|--------------------------|
| Entity    | 0                 | 0                 | -                        | -                        |
| Card      | 0                 | 0                 | -                        | -                        |
| Character | 0                 | 0                 | -                        | -                        |
| Combat    | -                 | -                 | 0                        | 0                        |
| Game Flow | -                 | -                 | 0                        | 0                        |

## Test Deliverables

1. Test code for each component
2. Test results summary
3. Fixed bugs identified during testing 