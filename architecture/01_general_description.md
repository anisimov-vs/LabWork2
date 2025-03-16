# General System Description

## Overview

Deckstiny is a turn-based roguelike card game inspired by the popular game "Slay the Spire." Players navigate through a procedurally generated map, engaging in strategic card-based combat encounters with various enemies, collecting relics that provide special abilities, and making choices during random events.

## Core Game Concept

Players choose a character class with unique starting abilities and cards, then proceed through a series of encounters represented on a map. The primary gameplay loop involves:

1. **Map Navigation**: Players choose paths through a procedurally generated map, each containing different types of encounters.
2. **Card-Based Combat**: Players use cards from their deck to attack enemies, defend against attacks, and apply various effects.
3. **Deck Building**: As players progress, they acquire new cards to strengthen their deck.
4. **Collection of Relics**: Special items that provide passive bonuses throughout a run.
5. **Resource Management**: Players must manage their health, energy (used to play cards), and gold.

## Architecture Philosophy

The game is designed with the following architectural principles:

### 1. Extensibility

The system is built to be highly extensible, allowing for easy addition of:
- New character classes
- New cards
- New enemies
- New relics
- New events

### 2. Data-Driven Design

Game content (cards, enemies, relics, etc.) is defined in external JSON files, allowing for:
- Easy modification without changing code
- Potential for modding support
- Clear separation between game logic and content

### 3. Separation of Concerns

The codebase maintains strict separation between:
- Core game logic
- UI representation
- Data handling
- Game state management

### 4. Abstraction Layers

The UI is abstracted through interfaces, allowing for different implementations (text-based, graphical) without changing the core game logic.

## Technical Implementation

The game is implemented in C++17, making use of:
- Object-oriented design with inheritance hierarchies
- Smart pointers for memory management
- JSON for data serialization and storage
- CMake for build system management

The current implementation provides a text-based interface, with the architecture designed to support a graphical interface in the future. 