# Additional Functionality Report

## 1. Overview

This document details the additional functionality implemented in the Deckstiny project, covering enhancements to the user experience, deployment automation, and cross-platform compatibility.

## 2. Graphical User Interface (GUI)

### 2.1 UI Framework

The Deckstiny game implements a robust and intuitive graphical user interface using **SFML (Simple and Fast Multimedia Library)**. This choice provides several benefits:

-   **Cross-platform Rendering**: Ensures a consistent visual experience across various supported operating systems.
-   **Hardware Acceleration**: Utilizes GPU-based rendering for smooth animations and efficient performance.
-   **Custom UI Components**: Enables the creation of bespoke game-specific interface elements tailored to Deckstiny's unique mechanics.
-   **Responsive Layout**: Designed to adapt and display correctly across different screen resolutions and aspect ratios.

### 2.2 Game Interface Elements

The graphical interface meticulously presents key game information and interaction points:

-   **Main Menu**: Provides an engaging entry point with options to start a new game, load, or exit.
-   **Combat View**: A dynamic display showing player and enemy health, status effects, current energy, available cards in hand, and enemy intents. This includes visual indicators and clearer text descriptions for complex intents like "attack_debuff" and "defend_debuff".
-   **Map Screen**: An interactive dungeon map, allowing visual navigation between different room types (e.g., Monster, Elite, Boss, Shop, Event, Rest Site).
-   **Shop Screen**: A visual marketplace displaying cards and relics for sale, correctly showing their gold prices (distinct from energy costs) and indicating affordability.
-   **Card Management & Upgrade**: Provides a visual interface for viewing the player's deck, and a modal system for selecting cards for upgrade at rest sites, ensuring the correct card is chosen.
-   **Rest Site**: Displays healing and upgrading options with proper visual feedback and keyboard navigation.
-   **Event & Rewards Screens**: Presents event choices and combat rewards clearly, using a dedicated overlay system to ensure messages (including event results and combat rewards) are displayed and acknowledged before the game state transitions.
-   **Status Displays**: Clear health bars, energy indicators, block values, and status effect icons/numbers for both player and enemies.

### 2.3 Visual Feedback and User Experience

Significant effort was invested in improving the visual feedback and overall user experience:

-   **Intuitive Navigation**: Consistent keyboard navigation (arrow keys, Enter/Space, number keys, Escape) across all interactive screens (menus, map, combat, shop, events, rest sites, card/relic views).
-   **Clear Information Hierarchy**: Information is presented logically, making it easy to discern critical game data.
-   **Overlay System**: A flexible overlay system was implemented to display messages (like event results, generic messages, or error notifications) on top of the current screen, ensuring they are always visible and acknowledged by the user. This also resolved issues where messages might be skipped due to rapid state transitions.
-   **Font Handling**: Robust font loading ensures that text is always displayed correctly, regardless of the application's launch environment (local build, AppImage).
-   **Aesthetic Improvements**: Positioning and formatting of UI elements (e.g., card numbers, costs, overlaid text messages with word wrapping) were refined to prevent overlap and enhance readability.

## 3. AppImage Packaging

### 3.1 Implementation Details

The Deckstiny game is now packaged as an **AppImage** for Linux distribution. This self-contained package format offers several advantages:

-   **No Installation Required**: Users can simply download and run the executable.
-   **Dependency Bundling**: All required shared libraries (including SFML) are included within the package, reducing external dependencies.
-   **Portability**: Works across various Linux distributions without needing specific package managers.
-   **Desktop Integration**: Provides proper application menu entries and icons on compatible desktop environments.

### 3.2 Build Process

The AppImage creation process is fully automated through GitHub Actions and includes:

1.  **Build**: The application is compiled with all required libraries.
2.  **`linuxdeploy`**: This tool is used to analyze the executable and its dependencies.
3.  **AppDir Creation**: A structured `AppDir` directory is created, containing the executable, its bundled dependencies, assets, and metadata.
4.  **Resource Bundling**: Game data files (`data/` directory and `arial.ttf` font) are copied into `AppDir/usr/share/resources/`.
5.  **Metadata Generation**: A `.desktop` entry file and an application icon (`deckstiny.png`) are created and copied to ensure proper desktop integration.
6.  **AppImage Generation**: `linuxdeploy` then bundles the `AppDir` into a single executable AppImage file.

### 3.3 Runtime Resource Management

Sophisticated runtime path resolution was implemented to ensure the game finds its resources and manages user data correctly across various environments:

-   **Data Path Priority**: `get_data_path_prefix()` prioritizes locating data files in this order:
    1.  User-specific modding directory (`~/.local/share/Deckstiny/data/`)
    2.  Current Working Directory (`./data/`)
    3.  AppImage's internal bundled data (`$APPDIR/usr/share/resources/data/`)
    4.  Executable-relative `data/` (e.g., `../data/` if placed next to the AppImage)
    5.  Relative paths (`../data/`, `../../data/`)
-   **First-Run Setup**: On the first run of the AppImage, if the user-specific data directory (`~/.local/share/Deckstiny/data`) does not exist, the bundled default `data` folder is recursively copied to this location. This enables users to easily mod the game by placing their own assets in this directory.
-   **Logging Management**: Log files are now correctly directed to `~/.local/share/Deckstiny/logs/` when running as an AppImage, preventing clutter in the current working directory. For development builds, logs remain in `logs/deckstiny` relative to the project root.
-   **Read-Only AppImage**: The game is designed to not attempt to create or modify directories within the read-only AppImage structure.

## 4. GitHub Release Automation

### 4.1 Automated Workflow

A comprehensive CI/CD pipeline was implemented using GitHub Actions, ensuring continuous delivery:

1.  **Build and Test**: The application is built and tested on every `push` to any branch.
2.  **AppImage Generation**: An AppImage package is created upon successful build.
3.  **Release Creation**: A GitHub release is automatically created for each successful build.
4.  **Artifact Attachment**: The generated AppImage is attached as a release asset, making it easily downloadable.

### 4.2 Release Versioning

A sophisticated versioning scheme ensures chronological sorting and clear identification of pre-releases:

-   **Format**: `Pre-release-Deckstiny-YYYY.MM.DD.HH.MM.SS-BRANCH_NAME-COMMITHASH`
-   **Chronological Sorting**: Uses a `YYYY.MM.DD.HH.MM.SS` timestamp format (e.g., `2025.06.02.21.41.29`) for accurate alphabetical sorting in GitHub's releases UI, ensuring newer builds always appear at the top.
-   **Traceability**: Includes the short commit hash for direct linking to the source code.
-   **Context**: The branch name from which the build originated is included for clarity in pre-releases.

### 4.3 Release Metadata

Each automated release includes rich metadata for better context and traceability:

-   **Pre-release Designation**: Clearly marked as a pre-release (`prerelease: true`) to distinguish development builds from stable releases.
-   **Release Name**: A human-readable yet sortable name as described above.
-   **Body/Notes**: Contains the latest commit message to indicate changes, along with the commit hash and branch name.
-   **Changelog Link**: Provides a direct link to the full changelog on GitHub, comparing the current commit with the previous release.

## 5. Cross-Platform Compatibility & Robustness

### 5.1 Runtime Environment Adaptations

The application's runtime behavior was made more robust to adapt to different environments:

-   **SFML Version**: The CI/CD pipeline ensures SFML 2.6.1 is used consistently across builds.
-   **Error Handling**: Improved logging and error messages provide better diagnostic information for issues like missing enemies, unplayable cards, or unknown enemy intents.
-   **Input Handling**: Modal input loops were introduced for specific UI contexts (like card upgrades) to correctly handle synchronous input expected by the game core, preventing UI freezes or crashes.