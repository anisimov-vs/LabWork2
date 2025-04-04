# Deckstiny Architecture

This folder contains architecture documentation for the Deckstiny project.

## Class Hierarchy

- **Game**: Main game controller class
- **Card**: Base class for all cards
- **Character**: Base class for player characters
- **Enemy**: Base class for enemies
- **Relic**: Base class for relics
- **Event**: Base class for events
- **UI**: Interface for UI implementations
  - **TextUI**: Text-based UI implementation

## Directory Structure

- **src/**: Source code files
  - **core/**: Core game logic
  - **ui/**: UI implementations
- **include/**: Header files
  - **core/**: Core game headers
  - **ui/**: UI headers
- **data/**: JSON data files
  - **characters/**: Character definitions
  - **cards/**: Card definitions
  - **enemies/**: Enemy definitions
  - **relics/**: Relic definitions
  - **events/**: Event definitions
- **tests/**: Unit tests
- **architecture/**: Architecture documentation
