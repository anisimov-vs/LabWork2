// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#ifndef DECKSTINY_GRAPHICAL_UI_H
#define DECKSTINY_GRAPHICAL_UI_H

#include "ui/ui_interface.h"
#include "core/map.h"
#include "core/combat.h"
#include "core/enemy.h"
#include <SFML/Graphics.hpp>
#include <functional>
#include <memory>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>

namespace deckstiny {

class GraphicalUI : public UIInterface {
public:
    GraphicalUI();
    ~GraphicalUI() override;

    bool initialize(Game* game) override;
    void run() override;
    void shutdown() override;
    void setInputCallback(std::function<bool(const std::string&)> callback) override;

    void showMainMenu() override;
    void showCharacterSelection(const std::vector<std::string>& availableClasses) override;
    void showMap(int currentRoomId, const std::vector<int>& availableRooms, const std::unordered_map<int, Room>& allRooms) override;
    void showCombat(const Combat* combat) override;
    void showPlayerStats(const Player* player) override;
    void showEnemyStats(const Enemy* enemy) override;
    void showEnemySelectionMenu(const Combat* combat, const std::string& cardName) override;
    void showCard(const Card* card, bool showEnergyCost = true, bool selected = false) override;
    void showCards(const std::vector<Card*>& cards, const std::string& title = "", bool showIndices = true) override;
    void showRelic(const Relic* relic) override;
    void showRelics(const std::vector<Relic*>& relics, const std::string& title = "") override;
    void showMessage(const std::string& message, bool pause = false) override;
    std::string getInput(const std::string& prompt) override;
    void clearScreen() const override;
    void update() override;
    void showRewards(int gold, const std::vector<Card*>& cards, const std::vector<Relic*>& relics) override;
    void showGameOver(bool victory, int score) override;
    void showEvent(const Event* event, const Player* player) override;
    void showEventResult(const std::string& resultText) override;
    void showShop(const std::vector<Card*>& cardsForSale, const std::vector<Relic*>& relicsForSale, int playerGold) override;
    void showShop(const std::vector<Card*>& cards, 
                  const std::vector<Relic*>& relics,
                  const std::map<Relic*, int>& relicPrices, 
                  const std::map<Card*, int>& cardPrices,
                  int playerGold) override;

private:
    enum class ScreenType {
        None,
        MainMenu,
        CharacterSelect,
        Map,
        Combat,
        Message,
        Rewards,
        GameOver,
        Event,
        EventResult,
        Shop,
        CardView,
        CardsView,
        EnemySelection,
        RelicView,
        RelicsView,
        Rest
    };

    enum class OverlayType {
        None,
        EventResult,
        GenericMessage
    };

    void processEvent(const sf::Event& event);
    void draw();
    // Combat drawing helpers
    void drawPlayerInfoGfx(sf::RenderTarget& target, const sf::FloatRect& area);
    void drawEnemyInfoGfx(sf::RenderTarget& target, const Enemy* enemy, const sf::FloatRect& area);
    std::string getEnemyIntentStringGfx(const Intent& intent);
    void processModalCardSelectionEvent(const sf::Event& event);

    Game* game_ = nullptr;
    std::function<bool(const std::string&)> inputCallback_;
    sf::RenderWindow window_;
    sf::Font font_;
    ScreenType screenType_ = ScreenType::None;
    std::string title_;
    std::vector<std::string> options_;
    std::vector<std::string> optionInputs_;
    size_t selectedIndex_ = 0;
    std::string message_;
    int mapCurrentRoomId_ = -1;
    std::vector<int> mapAvailableRooms_;
    std::unordered_map<int, Room> mapAllRooms_;
    const Combat* combat_ = nullptr;
    struct ShopItemDisplay {
        std::string name;
        std::string displayString; // Pre-formatted string for drawing
        std::string originalInputString; // "C1", "R1", "leave", etc.
        int price = 0;
        bool canAfford = true;
        const Card* cardPtr = nullptr;
        const Relic* relicPtr = nullptr;
    };
    std::vector<ShopItemDisplay> shopDisplayItems_;
    int shopPlayerGold_ = 0;
    bool isShowingRewardsOverlay_ = false;
    int rewardsGoldValue_ = 0; // For specific gold display on rewards screen
    
    // Card display state
    const Card* cardToDisplay_ = nullptr;
    bool showCardEnergyCost_ = true;
    bool isCardSelected_ = false;
    std::vector<const Card*> cardsToDisplay_;
    bool showCardIndices_ = true;
    
    // Relic display state
    const Relic* relicToDisplay_ = nullptr;
    std::vector<const Relic*> relicsToDisplay_;

    // Overlay System Members
    OverlayType currentOverlay_ = OverlayType::None;
    std::string overlayTitleText_;
    std::string overlayMessageText_;

    // Store the current event when showEvent is called for context in showEventResult
    const Event* currentEventForTitle_ = nullptr;

    // Members for modal card selection (e.g., for upgrade)
    bool isAwaitingModalCardSelection_ = false;
    std::string modalSelectedCardInput_ = "";
    std::string modalUpgradePrompt_ = "";

    std::vector<sf::Text> mapNodeTexts_;
    std::vector<sf::CircleShape> mapNodeShapes_;
};

} // namespace deckstiny

#endif // DECKSTINY_GRAPHICAL_UI_H 