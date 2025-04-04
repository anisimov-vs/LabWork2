# Deckstiny - Laboratory Work 2

![Build Status](https://github.com/anisimov-vs/LabWork2/actions/workflows/build.yml/badge.svg)

## Author

Анисимов Василий Сергеевич, группа 24.Б81-мм

## Contacts

st129629@student.spbu.ru

## Description

Deckstiny is a C++ implementation of a roguelike deck-building game inspired by "Slay the Spire." This project develops a comprehensive game core featuring character selection, strategic card play, and procedurally generated levels with combat, events, and relics. The architecture includes a hierarchy of over 20 classes, multiple interaction modes, and enemy AI, forming a solid foundation for further development.

The game features:
- Multiple character classes with unique starting decks and abilities
- A wide variety of cards and relics to collect
- Different enemy types with distinct combat behaviors
- Procedurally generated maps with various encounter types
- Text-based UI (with architecture to support future graphical UI)

For detailed architectural information, see the [architecture folder](architecture/).

### Architecture

The project is designed with separation of concerns and extensibility in mind:

- **Core Game Logic**: All game mechanics and entities are implemented as separate classes
- **Data-Driven Design**: Game entities (cards, enemies, relics, etc.) are defined in JSON files for easy modification
- **UI Abstraction**: The UI interface is abstracted, allowing for easy switching between text-based and graphical implementations
- **Clear Hierarchy**: There's a clear inheritance hierarchy that promotes code reuse

### Directory Structure

- `src/`: Source code files
  - `core/`: Core game logic
  - `ui/`: UI implementations
- `include/`: Header files
- `data/`: JSON data files
  - `characters/`: Character definitions
  - `cards/`: Card definitions
  - `enemies/`: Enemy definitions
  - `relics/`: Relic definitions
  - `events/`: Event definitions
- `tests/`: Unit tests
- `architecture/`: Architecture documentation

### Documentation

Source code documentation is generated using Doxygen. You can view the documentation by opening the following file in your web browser:

[Documentation](docs/doxygen/html/index.html)

### Build Instructions

#### Prerequisites

- C++17 compatible compiler
- CMake 3.10 or higher

#### Build

```bash
mkdir build
cd build
cmake ..
make
```

### Run

```bash
./deckstiny
```

### Extending the Game

#### Adding New Cards

Create a new JSON file in the `data/cards/` directory with the appropriate card properties.

#### Adding New Enemies

Create a new JSON file in the `data/enemies/` directory with enemy properties and behavior.

#### Adding New Relics

Create a new JSON file in the `data/relics/` directory with relic properties and effects.

### Usage

The game features a text-based interface. Upon starting:

1. Select a character class
2. Navigate through the map
3. Engage in card-based combat encounters
4. Collect cards and relics to build your deck
5. Defeat the final boss to win

More detailed game instructions will be provided as the project develops.

## License

This project is provided as-is for educational purposes.
