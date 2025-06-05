// Anisimov Vasiliy st129629@student.spbu.ru
// Laboratory Work 2

#include "ui/graphical_ui.h"
#include "core/game.h"
#include "core/combat.h"
#include "core/player.h"
#include "core/enemy.h"
#include "core/card.h"
#include "core/relic.h"
#include "core/event.h"
#include "util/logger.h"
#include "util/path_util.h" 
#include <SFML/Window/Event.hpp>
#include <set>
#include <vector>
#include <string>
#include <map>
#include <queue>
#include <algorithm>
#include <cmath>
#include <sstream>
#include <filesystem>

namespace deckstiny {

// Define constants for card display
const float CARD_WIDTH_GFX = 220.f;
const float CARD_SPACING_GFX = 20.f;
const float RELIC_WIDTH_GFX = 200.f;
const float RELIC_SPACING_GFX = 20.f;

// Helper to get card type string, similar to TextUI
std::string getCardTypeStringGfx(CardType type) {
    switch (type) {
        case CardType::ATTACK:  return "Attack";
        case CardType::SKILL:   return "Skill";
        case CardType::POWER:   return "Power";
        case CardType::STATUS:  return "Status";
        case CardType::CURSE:   return "Curse";
        default:                return "Unknown";
    }
}

// Helper to get enemy intent string, similar to TextUI
std::string GraphicalUI::getEnemyIntentStringGfx(const Intent& intent) {
    std::stringstream ss;
    if (intent.type == "attack") {
        ss << "Attack: " << intent.value;
    } else if (intent.type == "attack_defend") {
        ss << "Attack: " << intent.value << ", Defend: " << intent.secondaryValue;
    } else if (intent.type == "defend") {
        ss << "Defend: " << intent.value;
    } else if (intent.type == "buff") {
        ss << "Buff";
        if (!intent.effect.empty()) {
            ss << " (" << intent.effect << " +" << intent.value << ")";
        }
    } else if (intent.type == "debuff") {
        ss << "Debuff";
        if (!intent.effect.empty()) {
            ss << " (" << intent.effect << " +" << intent.value << ")";
        }
    } else if (intent.type == "attack_debuff") {
        ss << "Attack: " << intent.value;
        if (!intent.effect.empty()) {
            ss << ", Debuff (" << intent.effect;
            if (intent.secondaryValue > 0) {
                ss << " " << intent.secondaryValue;
            }
            ss << ")";
        } else {
            ss << ", Debuff";
        }
    } else if (intent.type == "defend_debuff") {
        ss << "Defend: " << intent.value;
        if (!intent.effect.empty()) {
            ss << ", Debuff (" << intent.effect;
            if (intent.secondaryValue > 0) {
                ss << " " << intent.secondaryValue;
            }
            ss << ")";
        } else {
            ss << ", Debuff";
        }
    } else {
        ss << intent.type;
        if (intent.value > 0) {
            ss << " (" << intent.value << ")";
        }
    }
    return ss.str();
}

void GraphicalUI::showPlayerStats(const Player* player) {
    (void)player;
}

void GraphicalUI::showEnemyStats(const Enemy* enemy) {
    (void)enemy;
}

void GraphicalUI::drawPlayerInfoGfx(sf::RenderTarget& target, const sf::FloatRect& area) {
    if (!combat_ || !combat_->getPlayer()) return;
    const Player* player = combat_->getPlayer();
    float padding = 10.f;
    float currentY = area.top + padding;
    float lineSpacing = 22.f;
    unsigned int charSize = 16;

    // Draw background for player area
    sf::RectangleShape bgRect(sf::Vector2f(area.width, area.height));
    bgRect.setPosition(area.left, area.top);
    bgRect.setFillColor(sf::Color(50, 50, 50, 150));
    bgRect.setOutlineColor(sf::Color(100, 100, 100));
    bgRect.setOutlineThickness(1.f);
    target.draw(bgRect);

    sf::Text text;
    text.setFont(font_);
    text.setCharacterSize(charSize);
    text.setFillColor(sf::Color::White);

    // Name
    text.setString(player->getName());
    sf::FloatRect textBounds_name = text.getLocalBounds();
    text.setPosition(area.left + (area.width - textBounds_name.width) / 2.0f - textBounds_name.left, currentY);
    target.draw(text);
    currentY += lineSpacing;

    // HP
    text.setString("HP: " + std::to_string(player->getHealth()) + " / " + std::to_string(player->getMaxHealth()));
    sf::FloatRect textBounds_hp = text.getLocalBounds();
    text.setPosition(area.left + (area.width - textBounds_hp.width) / 2.0f - textBounds_hp.left, currentY);
    target.draw(text);
    currentY += lineSpacing;

    // Block
    if (player->getBlock() > 0) {
        text.setString("Block: " + std::to_string(player->getBlock()));
        sf::FloatRect textBounds_block = text.getLocalBounds();
        text.setPosition(area.left + (area.width - textBounds_block.width) / 2.0f - textBounds_block.left, currentY);
        target.draw(text);
        currentY += lineSpacing;
    }

    // Energy
    text.setString("Energy: " + std::to_string(player->getEnergy()) + " / " + std::to_string(player->getBaseEnergy()));
    sf::FloatRect textBounds_energy = text.getLocalBounds();
    text.setPosition(area.left + (area.width - textBounds_energy.width) / 2.0f - textBounds_energy.left, currentY);
    target.draw(text);
    currentY += lineSpacing;

    // Status Effects
    const auto& effects = player->getStatusEffects();
    if (!effects.empty()) {
        currentY += lineSpacing * 0.5f;
        text.setString("Effects:");
        sf::FloatRect textBounds_effects_title = text.getLocalBounds();
        text.setPosition(area.left + (area.width - textBounds_effects_title.width) / 2.0f - textBounds_effects_title.left, currentY);
        target.draw(text);
        currentY += lineSpacing;
        for (const auto& effect : effects) {
            if (currentY + charSize > area.top + area.height - padding) break;
            text.setString("  " + effect.first + ": " + std::to_string(effect.second));
            sf::FloatRect textBounds_effect = text.getLocalBounds();
            text.setPosition(area.left + (area.width - textBounds_effect.width) / 2.0f - textBounds_effect.left, currentY);
            target.draw(text);
            currentY += lineSpacing;
        }
    }
}

void GraphicalUI::drawEnemyInfoGfx(sf::RenderTarget& target, const Enemy* enemy, const sf::FloatRect& area) {
    if (!enemy) return;
    float padding = 10.f;
    float currentY = area.top + padding;
    float lineSpacing = 22.f;
    unsigned int charSize = 16;

    // Draw background for enemy area
    sf::RectangleShape bgRect(sf::Vector2f(area.width, area.height));
    bgRect.setPosition(area.left, area.top);
    bgRect.setFillColor(sf::Color(60, 40, 40, 150));
    bgRect.setOutlineColor(sf::Color(100, 80, 80));
    bgRect.setOutlineThickness(1.f);
    target.draw(bgRect);

    sf::Text text;
    text.setFont(font_);
    text.setCharacterSize(charSize);
    text.setFillColor(sf::Color::White);

    // Name
    text.setString(enemy->getName());
    sf::FloatRect textBounds_enemy_name = text.getLocalBounds();
    text.setPosition(area.left + (area.width - textBounds_enemy_name.width) / 2.0f - textBounds_enemy_name.left, currentY);
    target.draw(text);
    currentY += lineSpacing;

    // HP
    text.setString("HP: " + std::to_string(enemy->getHealth()) + " / " + std::to_string(enemy->getMaxHealth()));
    sf::FloatRect textBounds_enemy_hp = text.getLocalBounds();
    text.setPosition(area.left + (area.width - textBounds_enemy_hp.width) / 2.0f - textBounds_enemy_hp.left, currentY);
    target.draw(text);
    currentY += lineSpacing;

    // Block
    if (enemy->getBlock() > 0) {
        text.setString("Block: " + std::to_string(enemy->getBlock()));
        sf::FloatRect textBounds_enemy_block = text.getLocalBounds();
        text.setPosition(area.left + (area.width - textBounds_enemy_block.width) / 2.0f - textBounds_enemy_block.left, currentY);
        target.draw(text);
        currentY += lineSpacing;
    }

    // Intent
    text.setString("Intent: " + getEnemyIntentStringGfx(enemy->getIntent()));
    sf::FloatRect textBounds_enemy_intent = text.getLocalBounds();
    text.setPosition(area.left + (area.width - textBounds_enemy_intent.width) / 2.0f - textBounds_enemy_intent.left, currentY);
    target.draw(text);
    currentY += lineSpacing;

    // Status Effects
    const auto& effects = enemy->getStatusEffects();
    if (!effects.empty()) {
        currentY += lineSpacing * 0.5f;
        text.setString("Effects:");
        sf::FloatRect textBounds_enemy_effects_title = text.getLocalBounds();
        text.setPosition(area.left + (area.width - textBounds_enemy_effects_title.width) / 2.0f - textBounds_enemy_effects_title.left, currentY);
        target.draw(text);
        currentY += lineSpacing;
        for (const auto& effect : effects) {
            if (currentY + charSize > area.top + area.height - padding) break;
            text.setString("  " + effect.first + ": " + std::to_string(effect.second));
            sf::FloatRect textBounds_enemy_effect = text.getLocalBounds();
            text.setPosition(area.left + (area.width - textBounds_enemy_effect.width) / 2.0f - textBounds_enemy_effect.left, currentY);
            target.draw(text);
            currentY += lineSpacing;
        }
    }
}

GraphicalUI::GraphicalUI() {}

GraphicalUI::~GraphicalUI() {
    shutdown();
}

bool GraphicalUI::initialize(Game* game) {
    game_ = game;
    LOG_INFO("graphical_ui", "Initializing Graphical UI");
    window_.create(sf::VideoMode(1280, 720), "Deckstiny");
    window_.setVisible(true);
    LOG_DEBUG("graphical_ui", "Window created with size: " + std::to_string(window_.getSize().x) + "x" + std::to_string(window_.getSize().y));
    window_.setFramerateLimit(60);

    std::string data_prefix = get_data_path_prefix();
    std::string font_path_str = data_prefix + "data/assets/arial.ttf";
    LOG_INFO("graphical_ui", "Attempting to load font from: " + font_path_str);

    if (!font_.loadFromFile(font_path_str)) {
        LOG_ERROR("graphical_ui", "Failed to load font from: " + font_path_str + ". Attempting fallback.");
        std::string fallback_font_path = "arial.ttf";
        if (!font_.loadFromFile(fallback_font_path)) {
             LOG_ERROR("graphical_ui", "Failed to load font from fallback path: " + fallback_font_path + ". Text may not display.");
        } else {
            LOG_INFO("graphical_ui", "Successfully loaded font from fallback path: " + fallback_font_path);
        }
    } else {
        LOG_INFO("graphical_ui", "Successfully loaded font from: " + font_path_str);
    }
    return true;
}

void GraphicalUI::setInputCallback(std::function<bool(const std::string&)> callback) {
    inputCallback_ = callback;
}

void GraphicalUI::run() {
    LOG_INFO("graphical_ui", "Graphical UI run started");
    LOG_DEBUG("graphical_ui", "run() entered; window_ is open=" + std::string(window_.isOpen() ? "true" : "false") + ", game running=" + std::string(game_->isRunning() ? "true" : "false"));
    // Main UI loop: run while window is open
    while (window_.isOpen()) {
        LOG_DEBUG("graphical_ui", "run loop iteration; window open=" + std::string(window_.isOpen() ? "true" : "false") + ", game running=" + std::string(game_->isRunning() ? "true" : "false"));
        sf::Event event;
        while (window_.pollEvent(event)) {
            processEvent(event);
        }
        draw();
        window_.display();
        // If game has ended, close window
        if (!game_->isRunning()) {
            LOG_DEBUG("graphical_ui", "Game no longer running. Closing window.");
            window_.close();
        }
    }
}

void GraphicalUI::shutdown() {
    if (window_.isOpen()) {
        window_.close();
    }
}

void GraphicalUI::processEvent(const sf::Event& event) {
    LOG_DEBUG("graphical_ui_trace", "processEvent: START, screenType_ = " + std::to_string(static_cast<int>(screenType_)) + 
                                   ", overlayType = " + std::to_string(static_cast<int>(currentOverlay_)) +
                                   ", event_type = " + std::to_string(event.type));

    // --- Overlay Input Handling --- (PRIORITY 1)
    if (currentOverlay_ != OverlayType::None) {
        if (event.type == sf::Event::Closed) {
            if (game_) game_->shutdown();
        } else if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                LOG_DEBUG("graphical_ui_trace", "Overlay ACTIVE, Enter/Space pressed. Clearing overlay.");
                currentOverlay_ = OverlayType::None;
            }
        }
        if (event.type != sf::Event::Closed) return;
    }

    // --- Rewards Overlay Input Handling --- (PRIORITY 2 - to be potentially merged with above)
    if (isShowingRewardsOverlay_) {
        if (event.type == sf::Event::Closed) {
            if (game_) game_->shutdown();
        } else if (event.type == sf::Event::KeyPressed) {
            if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                LOG_DEBUG("graphical_ui_trace", "RewardsOverlay ACTIVE, Enter/Space pressed. Hiding overlay and setting screen to Map.");
                isShowingRewardsOverlay_ = false;
                screenType_ = ScreenType::Map;
            }
        }
        if (event.type != sf::Event::Closed) return;
    }

    if (event.type == sf::Event::Closed) {
        if (game_) {
            game_->shutdown();
        }
    } else if (event.type == sf::Event::KeyPressed) {
        switch (screenType_) {
            case ScreenType::MainMenu:
            case ScreenType::CharacterSelect:
            case ScreenType::Shop:
            {
                // Allow number key to select option directly
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                    int keyNum = event.key.code - sf::Keyboard::Num1;
                    if (keyNum < static_cast<int>(optionInputs_.size())) {
                        LOG_DEBUG("graphical_ui", "Shop: Number key " + std::to_string(keyNum + 1) + " pressed. Selected option: " + optionInputs_[keyNum]);
                        if (inputCallback_) inputCallback_(optionInputs_[keyNum]);
                        selectedIndex_ = keyNum;
                    }
                    break;
                }
                if (event.key.code == sf::Keyboard::Up) {
                    if (selectedIndex_ > 0) selectedIndex_--;
                } else if (event.key.code == sf::Keyboard::Down) {
                    if (selectedIndex_ + 1 < options_.size()) selectedIndex_++;
                } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                    if (inputCallback_ && selectedIndex_ < optionInputs_.size()) {
                        LOG_DEBUG("graphical_ui", "Shop: Enter pressed for option: " + optionInputs_[selectedIndex_]);
                        inputCallback_(optionInputs_[selectedIndex_]);
                    }
                } else if (event.key.code == sf::Keyboard::Escape) {
                    if (inputCallback_) {
                        auto it = std::find(optionInputs_.begin(), optionInputs_.end(), "leave");
                        if (it != optionInputs_.end()) {
                            LOG_DEBUG("graphical_ui", "Shop: Escape pressed, sending 'leave' command.");
                            inputCallback_("leave");
                        } else {
                            LOG_ERROR("graphical_ui", "Shop: Escape pressed, but no 'leave' optionInput found. Sending 'back'.");
                            inputCallback_("back");
                        }
                    }
                }
                break;
            }
            case ScreenType::Map:
            {
                // Allow number key to select option directly
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                    int idx = event.key.code - sf::Keyboard::Num1;
                    if (idx < static_cast<int>(optionInputs_.size())) {
                        LOG_DEBUG("graphical_ui", "Map: Number key pressed, selecting option " + optionInputs_[idx]);
                        if (inputCallback_) inputCallback_(optionInputs_[idx]);
                    }
                    break; 
                }
                if (event.key.code == sf::Keyboard::Left) { 
                    if (selectedIndex_ > 0) selectedIndex_--;
                    LOG_DEBUG("graphical_ui", "Map: Left pressed, selectedIndex_ = " + std::to_string(selectedIndex_));
                } else if (event.key.code == sf::Keyboard::Right) {
                    if (selectedIndex_ + 1 < mapAvailableRooms_.size()) selectedIndex_++; 
                    LOG_DEBUG("graphical_ui", "Map: Right pressed, selectedIndex_ = " + std::to_string(selectedIndex_));
                } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                    if (inputCallback_ && selectedIndex_ < optionInputs_.size() && selectedIndex_ < mapAvailableRooms_.size()) {
                         LOG_DEBUG("graphical_ui", "Map: Enter pressed, selectedIndex_ = " + std::to_string(selectedIndex_) + ", input: " + optionInputs_[selectedIndex_]);
                        inputCallback_(optionInputs_[selectedIndex_]);
                    }
                } else if (event.key.code == sf::Keyboard::Escape) {
                    if (inputCallback_) {
                         LOG_DEBUG("graphical_ui", "Map: Escape pressed");
                        inputCallback_("back");
                    }
                }
                break;
            }
            case ScreenType::Combat:
            {
                LOG_DEBUG("graphical_ui_combat_input", "Processing key press in Combat. selectedIndex_: " + std::to_string(selectedIndex_));
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                    int num = event.key.code - sf::Keyboard::Num1 + 1;
                    LOG_DEBUG("graphical_ui_combat_input", "Num key pressed: " + std::to_string(num));
                    if (static_cast<size_t>(num) <= options_.size()) {
                        if (static_cast<size_t>(num - 1) < optionInputs_.size() && !optionInputs_[num-1].empty()){
                             bool is_numeric_input = true;
                             for(char c : optionInputs_[num-1]) { if(!isdigit(c)) {is_numeric_input = false; break;} }

                            if(is_numeric_input && std::stoi(optionInputs_[num-1]) == num){
                                LOG_DEBUG("graphical_ui_combat_input", "Sending card input: " + optionInputs_[num-1]);
                                if (inputCallback_) inputCallback_(optionInputs_[num-1]);
                            } 
                        }
                    }
                } else if (event.key.code == sf::Keyboard::E) {
                    LOG_DEBUG("graphical_ui_combat_input", "Sending 'end' input.");
                    if (inputCallback_) inputCallback_("end");
                } else if (event.key.code == sf::Keyboard::Up) {
                    if (selectedIndex_ > 0) selectedIndex_--;
                    LOG_DEBUG("graphical_ui_combat_input", "Up arrow. new selectedIndex_: " + std::to_string(selectedIndex_));
                } else if (event.key.code == sf::Keyboard::Down) {
                    if (selectedIndex_ + 1 < options_.size()) selectedIndex_++;
                    LOG_DEBUG("graphical_ui_combat_input", "Down arrow. new selectedIndex_: " + std::to_string(selectedIndex_));
                } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                    if (inputCallback_ && selectedIndex_ < optionInputs_.size()) {
                        LOG_DEBUG("graphical_ui_combat_input", "Enter pressed. Sending input: " + optionInputs_[selectedIndex_] + " for selectedIndex: " + std::to_string(selectedIndex_));
                        inputCallback_(optionInputs_[selectedIndex_]);
                    }
                }
                break;
            }
            case ScreenType::Message:
            case ScreenType::EventResult:
            {
                if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                    if (inputCallback_) {
                        LOG_DEBUG("graphical_ui", "Message/EventResult screen - Enter/Space pressed. Sending 'continue'.");
                        inputCallback_("continue");
                    }
                }
                break;
            }
            case ScreenType::GameOver:
            {
                LOG_DEBUG("graphical_ui_trace", "processEvent: In GameOver case. Current screenType_ = " + std::to_string(static_cast<int>(screenType_)) + ", event_type = " + std::to_string(event.type));
                if (event.type == sf::Event::KeyPressed) { 
                    if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                        LOG_INFO("graphical_ui", "GameOver screen: Enter/Space pressed. Sending 'end' to Game::processInput to transition to main menu.");
                        LOG_DEBUG("graphical_ui_trace", "processEvent: ScreenType::GameOver (confirmed by switch), Enter/Space pressed. Calling inputCallback_ with 'end' argument.");
                        if (inputCallback_) inputCallback_("end");
                    }
                }
                break;
            }
            case ScreenType::Event:
            {
                LOG_DEBUG("graphical_ui", "Event screen: Key pressed " + std::to_string(event.key.code));
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                    int num = event.key.code - sf::Keyboard::Num1;
                    if (num < static_cast<int>(optionInputs_.size())) {
                        LOG_DEBUG("graphical_ui", "Event: Number key " + std::to_string(num + 1) + " pressed, sending: " + optionInputs_[num]);
                        if (inputCallback_) inputCallback_(optionInputs_[num]);
                    } else {
                        LOG_DEBUG("graphical_ui", "Event: Number key " + std::to_string(num + 1) + " out of bounds.");
                    }
                } else if (event.key.code == sf::Keyboard::Up) {
                    if (selectedIndex_ > 0) {
                        selectedIndex_--;
                        LOG_DEBUG("graphical_ui", "Event: Up arrow pressed, new selectedIndex_: " + std::to_string(selectedIndex_));
                    }
                } else if (event.key.code == sf::Keyboard::Down) {
                    if (selectedIndex_ + 1 < options_.size()) {
                        selectedIndex_++;
                        LOG_DEBUG("graphical_ui", "Event: Down arrow pressed, new selectedIndex_: " + std::to_string(selectedIndex_));
                    }
                } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                    if (inputCallback_ && selectedIndex_ < optionInputs_.size()) {
                        LOG_DEBUG("graphical_ui", "Event: Enter/Space pressed with option " + 
                                  std::to_string(selectedIndex_ + 1) + ", sending: " + optionInputs_[selectedIndex_]);
                        inputCallback_(optionInputs_[selectedIndex_]);
                    }
                } else if (event.key.code == sf::Keyboard::Escape) {
                    LOG_DEBUG("graphical_ui", "Event: Escape pressed, sending 'back'");
                    if (inputCallback_) inputCallback_("back");
                }
                break;
            }
            case ScreenType::Rest:
            {
                LOG_DEBUG("graphical_ui", "Rest site: Key pressed");
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                    int num = event.key.code - sf::Keyboard::Num1 + 1;
                    if (num <= static_cast<int>(optionInputs_.size())) {
                        LOG_DEBUG("graphical_ui", "Rest site: Number key " + std::to_string(num) + " pressed, sending: " + optionInputs_[num-1]);
                        if (inputCallback_) inputCallback_(optionInputs_[num-1]);
                    }
                } else if (event.key.code == sf::Keyboard::Up) {
                    if (selectedIndex_ > 0) selectedIndex_--;
                    LOG_DEBUG("graphical_ui", "Rest site: Up arrow pressed, new selectedIndex_: " + std::to_string(selectedIndex_));
                } else if (event.key.code == sf::Keyboard::Down) {
                    if (selectedIndex_ + 1 < options_.size()) selectedIndex_++;
                    LOG_DEBUG("graphical_ui", "Rest site: Down arrow pressed, new selectedIndex_: " + std::to_string(selectedIndex_));
                } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                    if (inputCallback_ && selectedIndex_ < optionInputs_.size()) {
                        LOG_DEBUG("graphical_ui", "Rest site: Enter/Space pressed with option " + 
                                  std::to_string(selectedIndex_+1) + ", sending: " + optionInputs_[selectedIndex_]);
                        inputCallback_(optionInputs_[selectedIndex_]);
                    }
                } else if (event.key.code == sf::Keyboard::Escape) {
                    LOG_DEBUG("graphical_ui", "Rest site: Escape pressed, sending 'leave'");
                    if (inputCallback_) inputCallback_("leave");
                }
                break;
            }
            case ScreenType::EnemySelection:
            {
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                    int num = event.key.code - sf::Keyboard::Num1 + 1;
                    if (num <= static_cast<int>(optionInputs_.size())) {
                        if (inputCallback_) inputCallback_(optionInputs_[num-1]);
                    }
                } else if (event.key.code == sf::Keyboard::Up) {
                    if (selectedIndex_ > 0) selectedIndex_--;
                } else if (event.key.code == sf::Keyboard::Down) {
                    if (selectedIndex_ + 1 < options_.size()) selectedIndex_++;
                } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                    if (inputCallback_ && selectedIndex_ < optionInputs_.size()) {
                        inputCallback_(optionInputs_[selectedIndex_]);
                    }
                } else if (event.key.code == sf::Keyboard::Escape) {
                    if (inputCallback_) inputCallback_("cancel");
                }
                break;
            }
            case ScreenType::CardView:
            {
                // Any key dismisses the card view
                if (inputCallback_) inputCallback_("continue");
                break;
            }
            case ScreenType::CardsView:
            {
                if (isAwaitingModalCardSelection_) {
                    LOG_DEBUG("graphical_ui", "processEvent: CardsView - modal is active, deferring to getInput loop.");
                    return; 
                }
                LOG_DEBUG("graphical_ui", "processEvent: CardsView - non-modal event: " + std::to_string(event.key.code));
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                    int num = event.key.code - sf::Keyboard::Num1 + 1;
                    if (static_cast<size_t>(num) <= optionInputs_.size()) {
                        if (inputCallback_ && (num -1) < static_cast<int>(optionInputs_.size())) inputCallback_(optionInputs_[num-1]);
                    }
                } else if (event.key.code == sf::Keyboard::Left) {
                    if (selectedIndex_ > 0) selectedIndex_--;
                } else if (event.key.code == sf::Keyboard::Right) {
                    if (selectedIndex_ + 1 < options_.size()) selectedIndex_++;
                } else if (event.key.code == sf::Keyboard::Up) {
                    size_t cardsPerRow = static_cast<size_t>(std::max(1, static_cast<int>((window_.getSize().x - 40) / (CARD_WIDTH_GFX + CARD_SPACING_GFX))));
                    if (selectedIndex_ >= cardsPerRow) selectedIndex_ -= cardsPerRow;
                } else if (event.key.code == sf::Keyboard::Down) {
                    size_t cardsPerRow = static_cast<size_t>(std::max(1, static_cast<int>((window_.getSize().x - 40) / (CARD_WIDTH_GFX + CARD_SPACING_GFX))));
                    if (selectedIndex_ + cardsPerRow < options_.size()) selectedIndex_ += cardsPerRow;
                } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                    if (inputCallback_ && selectedIndex_ < optionInputs_.size()) {
                        if (optionInputs_[selectedIndex_] == "close") {
                            inputCallback_("close");
                        } else {
                            LOG_DEBUG("graphical_ui", "Enter on non-modal CardsView, selected item: " + optionInputs_[selectedIndex_]);
                        }
                    }
                } else if (event.key.code == sf::Keyboard::Escape) {
                    if (inputCallback_) inputCallback_("close");
                }
                break;
            }
            case ScreenType::RelicView:
            {
                // Any key dismisses the relic view
                if (inputCallback_) inputCallback_("continue");
                break;
            }
            case ScreenType::RelicsView:
            {
                LOG_DEBUG("graphical_ui", "processEvent: RelicsView - event: " + std::to_string(event.key.code));
                if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
                    int num = event.key.code - sf::Keyboard::Num1 + 1;
                     if (static_cast<size_t>(num) <= optionInputs_.size()) {
                        if (inputCallback_ && (num-1) < static_cast<int>(optionInputs_.size())) inputCallback_(optionInputs_[num-1]);
                    }
                } else if (event.key.code == sf::Keyboard::Left) {
                    if (selectedIndex_ > 0) selectedIndex_--;
                } else if (event.key.code == sf::Keyboard::Right) {
                    if (selectedIndex_ + 1 < options_.size()) selectedIndex_++;
                } else if (event.key.code == sf::Keyboard::Up) {
                    size_t relicsPerRow = static_cast<size_t>(std::max(1, static_cast<int>((window_.getSize().x - 40) / (RELIC_WIDTH_GFX + RELIC_SPACING_GFX))));
                    if (selectedIndex_ >= relicsPerRow) selectedIndex_ -= relicsPerRow;
                } else if (event.key.code == sf::Keyboard::Down) {
                    size_t relicsPerRow = static_cast<size_t>(std::max(1, static_cast<int>((window_.getSize().x - 40) / (RELIC_WIDTH_GFX + RELIC_SPACING_GFX))));
                    if (selectedIndex_ + relicsPerRow < options_.size()) selectedIndex_ += relicsPerRow;
                } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
                    if (inputCallback_ && selectedIndex_ < optionInputs_.size()) {
                        if (optionInputs_[selectedIndex_] == "close") {
                            inputCallback_("close");
                        } else {
                            LOG_DEBUG("graphical_ui", "Enter on non-modal RelicsView, selected item: " + optionInputs_[selectedIndex_]);
                        }
                    }
                } else if (event.key.code == sf::Keyboard::Escape) {
                    if (inputCallback_) inputCallback_("close");
                }
                break;
            }
            default:
                break;
        }
    }
}

void GraphicalUI::showMainMenu() {
    screenType_ = ScreenType::MainMenu;
    title_ = "DECKSTINY";
    options_.clear(); optionInputs_.clear();
    options_.push_back("New Game"); optionInputs_.push_back("1");
    options_.push_back("Quit");    optionInputs_.push_back("2");
    selectedIndex_ = 0;
    combat_ = nullptr;
}

void GraphicalUI::showCharacterSelection(const std::vector<std::string>& availableClasses) {
    screenType_ = ScreenType::CharacterSelect;
    title_ = "CHARACTER SELECTION";
    options_.clear(); optionInputs_.clear();
    for (size_t i = 0; i < availableClasses.size(); ++i) {
        options_.push_back(availableClasses[i]);
        optionInputs_.push_back(std::to_string(i+1));
    }
    selectedIndex_ = 0;
}

void GraphicalUI::showMap(int currentRoomId, const std::vector<int>& availableRooms, const std::unordered_map<int, Room>& allRooms) {
    LOG_DEBUG("graphical_ui_trace", "GraphicalUI::showMap called. CurrentRoom: " + std::to_string(currentRoomId) + ", screenType_ will be set to Map");
    screenType_ = ScreenType::Map;
    title_ = "MAP";
    mapCurrentRoomId_ = currentRoomId;
    mapAvailableRooms_ = availableRooms;
    mapAllRooms_ = allRooms;
    options_.clear();
    optionInputs_.clear();
    for (size_t i = 0; i < mapAvailableRooms_.size(); ++i) {
        optionInputs_.push_back(std::to_string(i + 1));
    }
    selectedIndex_ = 0;
}

void GraphicalUI::showCombat(const Combat* combat) {
    LOG_DEBUG("graphical_ui_trace", "GraphicalUI::showCombat CALLED with combat pointer = " + std::string(combat ? "valid" : "nullptr") + ", previous screenType_ = " + std::to_string(static_cast<int>(screenType_)));
    
    if (screenType_ == ScreenType::GameOver && (title_ == "GAME OVER" || title_ == "VICTORY!")) {
        LOG_DEBUG("graphical_ui_trace", "showCombat called while in GameOver state. Ignoring to preserve game over screen.");
        return;
    }
    
    screenType_ = ScreenType::Combat;
    combat_ = combat;
    
    if (!combat_) {
        title_ = "COMBAT - ERROR";
        options_.clear();
        optionInputs_.clear();
        message_ = "Error: Combat data is missing.";
        selectedIndex_ = 0;
        LOG_ERROR("graphical_ui_trace", "Combat ERROR screen set due to null combat pointer. This should never happen in normal gameplay.");
        return;
    }

    title_ = "COMBAT - TURN " + std::to_string(combat_->getTurn());
    options_.clear();
    optionInputs_.clear();

    if (combat_->isPlayerTurn()) {
        const auto* player = combat_->getPlayer();
        if (player) {
            const auto& hand = player->getHand();
            for (size_t i = 0; i < hand.size(); ++i) {
                if (hand[i]) {
                    std::string cardText = "[" + std::to_string(hand[i]->getCost()) + "] ";
                    cardText += hand[i]->getName();
                    if (hand[i]->isUpgraded()) {
                        cardText += "+";
                    }
                    cardText += " (" + getCardTypeStringGfx(hand[i]->getType()) + ")";
                    options_.push_back(cardText);
                    optionInputs_.push_back(std::to_string(i + 1)); 
                }
            }
        }
        options_.push_back("End Turn");
        optionInputs_.push_back("end");
    }
    selectedIndex_ = 0;
}

void GraphicalUI::showEnemySelectionMenu(const Combat* combat, const std::string& cardName) {
    screenType_ = ScreenType::EnemySelection;
    combat_ = combat;
    title_ = "SELECT TARGET FOR " + cardName;
    options_.clear();
    optionInputs_.clear();
    selectedIndex_ = 0;

    if (!combat) return;
    
    for (size_t i = 0; i < combat->getEnemyCount(); ++i) {
        Enemy* enemy = combat->getEnemy(i);
        if (enemy && enemy->isAlive()) {
            options_.push_back(enemy->getName());
            optionInputs_.push_back(std::to_string(i + 1));
        }
    }
    
    options_.push_back("Cancel");
    optionInputs_.push_back("cancel");
}

void GraphicalUI::showCard(const Card* card, bool showEnergyCost, bool selected) {
    cardToDisplay_ = card;
    showCardEnergyCost_ = showEnergyCost;
    isCardSelected_ = selected;
    
    screenType_ = ScreenType::CardView;
    title_ = "Card Details";
}

void GraphicalUI::showCards(const std::vector<Card*>& cards, const std::string& title, bool showIndices) {
    LOG_DEBUG("graphical_ui", "showCards called. Title: '" + title + "', Card count: " + std::to_string(cards.size()));
    cardsToDisplay_.clear();
    for (const auto* card : cards) {
        cardsToDisplay_.push_back(card);
    }
    
    title_ = title.empty() ? "Cards" : title;
    showCardIndices_ = showIndices;
    
    screenType_ = ScreenType::CardsView;
    selectedIndex_ = 0;

    if (title.find("Select a card to upgrade") != std::string::npos) {
        LOG_INFO("graphical_ui", "showCards detected upgrade selection mode. Setting modal flags.");
        isAwaitingModalCardSelection_ = true;
        modalSelectedCardInput_ = "";
        modalUpgradePrompt_ = "Enter card number to upgrade";
    } else {
        isAwaitingModalCardSelection_ = false;
        modalUpgradePrompt_ = "";
    }
    
    options_.clear();
    optionInputs_.clear();
    
    if (!cards.empty()) {
        for (size_t i = 0; i < cards.size(); ++i) {
            if (showIndices) {
                options_.push_back(std::to_string(i + 1));
            } else {
                options_.push_back("");
            }
            optionInputs_.push_back(std::to_string(i + 1));
        }
    }
    
    options_.push_back("Close");
    optionInputs_.push_back("close");
}

void GraphicalUI::showRelic(const Relic* relic) {
    // Store the relic to display when draw() is called
    relicToDisplay_ = relic;
    
    // Set screen state to display a single relic
    screenType_ = ScreenType::RelicView;
    title_ = "Relic Details";
    
    // Clear previous options and set up a continue option
    options_.clear();
    optionInputs_.clear();
    options_.push_back("Continue");
    optionInputs_.push_back("continue");
    selectedIndex_ = 0;
}

void GraphicalUI::showRelics(const std::vector<Relic*>& relics, const std::string& title) {
    // Store relics for later drawing
    relicsToDisplay_.clear();
    for (const auto* relic : relics) {
        relicsToDisplay_.push_back(relic);
    }
    
    title_ = title.empty() ? "Relics" : title;
    
    // Set screen state to display multiple relics
    screenType_ = ScreenType::RelicsView;
    selectedIndex_ = 0;
    
    // Set up options for navigation
    options_.clear();
    optionInputs_.clear();
    
    if (!relics.empty()) {
        for (size_t i = 0; i < relics.size(); ++i) {
            options_.push_back(std::to_string(i + 1));
            optionInputs_.push_back(std::to_string(i + 1));
        }
    }
    
    options_.push_back("Close");
    optionInputs_.push_back("close");
}

void GraphicalUI::showMessage(const std::string& message, bool pause) {
    LOG_INFO("graphical_ui", "showMessage called with text: " + message);
    currentOverlay_ = OverlayType::GenericMessage;
    overlayTitleText_ = "MESSAGE"; 
    overlayMessageText_ = message + "\n\nPress Enter to continue.";
    (void)pause;
    
    LOG_INFO("graphical_ui", "Overlay set: type=GenericMessage, title=" + overlayTitleText_ + ", message=" + overlayMessageText_);
}

std::string GraphicalUI::getInput(const std::string& prompt) {
    LOG_DEBUG("graphical_ui", "getInput called with prompt: '" + prompt + "'. isAwaitingModalCardSelection_: " + (isAwaitingModalCardSelection_ ? "true" : "false"));

    if (isAwaitingModalCardSelection_ && !modalUpgradePrompt_.empty() && prompt.find(modalUpgradePrompt_) != std::string::npos) {
        LOG_INFO("graphical_ui", "Entering modal input loop for card upgrade selection.");
        modalSelectedCardInput_ = "";

        // Keep the window responsive and process events for this modal dialog
        while (window_.isOpen() && modalSelectedCardInput_.empty()) {
            sf::Event event;
            while (window_.pollEvent(event)) {
                if (event.type == sf::Event::Closed) {
                    if (game_) game_->shutdown();
                    window_.close();
                    modalSelectedCardInput_ = "cancel";
                    break; 
                }
                processModalCardSelectionEvent(event);
                if (!modalSelectedCardInput_.empty()) break;
            }

            if (!window_.isOpen() || !modalSelectedCardInput_.empty()) break; // Exit outer loop if window closed or selection made

            // Redraw the UI (which will show ScreenType::CardsView)
            draw();
            window_.display();
            
            // Give a small pause to prevent pegging CPU
            sf::sleep(sf::milliseconds(10)); 
        }

        LOG_INFO("graphical_ui", "Modal input loop finished. Selected input: '" + modalSelectedCardInput_ + "'");
        std::string capturedInput = modalSelectedCardInput_;
        
        // Reset modal flags
        isAwaitingModalCardSelection_ = false;
        modalSelectedCardInput_ = "";
        modalUpgradePrompt_ = "";
        
        return capturedInput.empty() ? "cancel" : capturedInput;
    }

    // Default non-blocking behavior for other getInput calls (should be rare for GraphicalUI)
    LOG_DEBUG("graphical_ui", "getInput: Not in modal selection mode or prompt mismatch. Returning empty string.");
    return "";
}

void GraphicalUI::clearScreen() const { }

void GraphicalUI::update() { }

void GraphicalUI::processModalCardSelectionEvent(const sf::Event& event) {
    if (event.type == sf::Event::KeyPressed) {
        LOG_DEBUG("graphical_ui_modal", "Modal CardsView KeyPress: " + std::to_string(event.key.code));
        if (event.key.code >= sf::Keyboard::Num1 && event.key.code <= sf::Keyboard::Num9) {
            int num = event.key.code - sf::Keyboard::Num1;
            if (num < static_cast<int>(cardsToDisplay_.size())) {
                modalSelectedCardInput_ = std::to_string(num + 1);
                LOG_INFO("graphical_ui_modal", "Modal selection: Card number " + modalSelectedCardInput_);
            }
        } else if (event.key.code == sf::Keyboard::Left) {
            if (selectedIndex_ > 0) selectedIndex_--;
        } else if (event.key.code == sf::Keyboard::Right) {
            if (selectedIndex_ + 1 < options_.size()) selectedIndex_++;
        } else if (event.key.code == sf::Keyboard::Up) {
            size_t cardsPerRow = static_cast<size_t>(std::max(1, static_cast<int>((window_.getSize().x - 40) / (CARD_WIDTH_GFX + CARD_SPACING_GFX))));
            if (selectedIndex_ >= cardsPerRow) selectedIndex_ -= cardsPerRow;
        } else if (event.key.code == sf::Keyboard::Down) {
            size_t cardsPerRow = static_cast<size_t>(std::max(1, static_cast<int>((window_.getSize().x - 40) / (CARD_WIDTH_GFX + CARD_SPACING_GFX))));
            if (selectedIndex_ + cardsPerRow < options_.size()) selectedIndex_ += cardsPerRow;
            else if (selectedIndex_ < options_.size() -1 ) selectedIndex_ = options_.size() -1;
        } else if (event.key.code == sf::Keyboard::Enter || event.key.code == sf::Keyboard::Space) {
            if (selectedIndex_ < cardsToDisplay_.size()) { 
                modalSelectedCardInput_ = std::to_string(selectedIndex_ + 1);
                LOG_INFO("graphical_ui_modal", "Modal selection: Card by Enter/Space " + modalSelectedCardInput_);
            } else {
                modalSelectedCardInput_ = "cancel";
                LOG_INFO("graphical_ui_modal", "Modal selection: Enter/Space with no valid card selected -> cancel");
            }
            isAwaitingModalCardSelection_ = false; // Exit modal state
        } else if (event.key.code == sf::Keyboard::Escape) {
            modalSelectedCardInput_ = "cancel";
            LOG_INFO("graphical_ui_modal", "Modal selection: Escape -> cancel");
        }
    }
}

void GraphicalUI::showRewards(int gold, const std::vector<Card*>& cards, const std::vector<Relic*>& relics) {
    screenType_ = ScreenType::Rewards;
    isShowingRewardsOverlay_ = true;
    title_ = "COMBAT REWARDS";
    rewardsGoldValue_ = gold;
    
    std::stringstream msg_ss;
    
    bool contentAdded = false;
    if (!cards.empty()) {
        msg_ss << "\nCard Rewards:\n";
        for (const Card* card : cards) {
            if (card) {
                msg_ss << "- " << card->getName();
                if (card->isUpgraded()) msg_ss << "+";
                msg_ss << "\n";
            }
        }
        contentAdded = true;
    }
    
    if (!relics.empty()) {
        msg_ss << "\nRelic Rewards:\n";
        for (const Relic* relic : relics) {
            if (relic) {
                msg_ss << "- " << relic->getName() << "\n";
            }
        }
        contentAdded = true;
    }
    
    if (contentAdded) {
        msg_ss << "\n";
    }
    msg_ss << "\nPress Enter to continue.";
    message_ = msg_ss.str();
    selectedIndex_ = 0; 
}

void GraphicalUI::showGameOver(bool victory, int score) {
    LOG_DEBUG("graphical_ui_trace", "GraphicalUI::showGameOver START. Current screenType_ = " + std::to_string(static_cast<int>(screenType_)));
    screenType_ = ScreenType::GameOver;
    LOG_DEBUG("graphical_ui_trace", "GraphicalUI::showGameOver AFTER set. New screenType_ = " + std::to_string(static_cast<int>(screenType_)) + ", Victory: " + std::string(victory ? "true" : "false") + ", Score: " + std::to_string(score) + ", isShowingRewardsOverlay_ = " + std::string(isShowingRewardsOverlay_ ? "true" : "false"));
    isShowingRewardsOverlay_ = false;
    title_ = victory ? "VICTORY!" : "GAME OVER";
    message_ = "Final Score: " + std::to_string(score) + "\n\nPress Enter to return to Main Menu.";
    options_.clear();
    optionInputs_.clear();
    selectedIndex_ = 0; 
    combat_ = nullptr;

    draw();
    window_.display();
    sf::Event dummyEvent;
    while (window_.pollEvent(dummyEvent)) { }
}

void GraphicalUI::showEvent(const Event* event, const Player* player) {
    if (!event) {
        LOG_ERROR("graphical_ui", "showEvent called with nullptr event");
        currentEventForTitle_ = nullptr;
        return;
    }
    currentEventForTitle_ = event;
    
    LOG_INFO("graphical_ui", "showEvent called with event: " + event->getName() + ", id: " + event->getId());
    
    if (event->getId() == "rest_site") {
        LOG_INFO("graphical_ui", "Rest site event detected, switching to Rest screen type");
        screenType_ = ScreenType::Rest;
        title_ = event->getName();
    } else {
        screenType_ = ScreenType::Event;
        title_ = "EVENT: " + event->getName();
    }

    options_.clear(); optionInputs_.clear();
    const auto& choices = event->getAvailableChoices(const_cast<Player*>(player));
    for (size_t i = 0; i < choices.size(); ++i) {
        options_.push_back(choices[i].text);
        optionInputs_.push_back(std::to_string(i+1));
        LOG_DEBUG("graphical_ui", "  Option " + std::to_string(i+1) + ": " + choices[i].text);
    }
    selectedIndex_ = 0;
}

void GraphicalUI::showEventResult(const std::string& resultText) {
    LOG_INFO("graphical_ui", "showEventResult called with text: " + resultText);
    LOG_INFO("graphical_ui", "  Current game title (before overlay): " + title_ + 
             ", screen type (before overlay): " + std::to_string(static_cast<int>(screenType_)));
    
    currentOverlay_ = OverlayType::EventResult;
    if (title_ == "Rest Site" || (currentEventForTitle_ && currentEventForTitle_->getId() == "rest_site")) {
        overlayTitleText_ = "Rest Site";
    } else {
        overlayTitleText_ = "EVENT RESULT";
    }
    overlayMessageText_ = resultText + "\n\nPress Enter to continue.";
    
    LOG_INFO("graphical_ui", "Overlay set: type=EventResult, title=" + overlayTitleText_ + ", message=" + overlayMessageText_);
}

void GraphicalUI::showShop(const std::vector<Card*>& cardsForSale, const std::vector<Relic*>& relicsForSale, int playerGold) {
    LOG_INFO("graphical_ui", "Simplified showShop (3-arg) called. Forwarding to 5-arg version with empty price maps.");
    showShop(cardsForSale, relicsForSale, {}, {}, playerGold);
}

void GraphicalUI::showShop(const std::vector<Card*>& cardsForSale, 
                           const std::vector<Relic*>& relicsForSale, 
                           const std::map<Relic*, int>& relicPricesFromGame,
                           const std::map<Card*, int>& cardPricesFromGame,
                           int playerGold) {
    screenType_ = ScreenType::Shop;
    shopPlayerGold_ = playerGold;
    title_ = "SHOP - Gold: " + std::to_string(playerGold) + "G";
    
    options_.clear();
    optionInputs_.clear();
    shopDisplayItems_.clear();
    selectedIndex_ = 0;

    size_t currentOptionIndex = 1;

    for (size_t i = 0; i < cardsForSale.size(); ++i) {
        if (!cardsForSale[i]) continue;
        Card* card = cardsForSale[i];
        ShopItemDisplay item;
        item.name = card->getName();
        if (card->isUpgraded()) item.name += "+";
        
        auto priceIt = cardPricesFromGame.find(card);
        if (priceIt != cardPricesFromGame.end()) {
            item.price = priceIt->second;
        } else {
            LOG_ERROR("graphical_ui", "Price not found for card: " + card->getName() + ". Setting price to 999.");
            item.price = 999;
        }
        
        item.canAfford = (playerGold >= item.price);
        item.cardPtr = card;
        item.originalInputString = "C" + std::to_string(i + 1);
        
        std::stringstream ss;
        ss << currentOptionIndex << ". [Card] " << item.name << " - " << item.price << "G";
        if (!item.canAfford) {
            ss << " (Unaffordable)";
        }
        item.displayString = ss.str();
        
        shopDisplayItems_.push_back(item);
        options_.push_back(item.displayString);
        optionInputs_.push_back(item.originalInputString);
        currentOptionIndex++;
    }

    // Process Relics
    for (size_t i = 0; i < relicsForSale.size(); ++i) {
        if (!relicsForSale[i]) continue;
        Relic* relic = relicsForSale[i];
        ShopItemDisplay item;
        item.name = relic->getName();
        
        auto priceIt = relicPricesFromGame.find(relic); 
        if (priceIt != relicPricesFromGame.end()) {
            item.price = priceIt->second;
        } else {
            LOG_ERROR("graphical_ui", "Price not found for relic: " + relic->getName() + ". Setting price to 999.");
            item.price = 999; 
        }
        
        item.canAfford = (playerGold >= item.price);
        item.relicPtr = relic;
        item.originalInputString = "R" + std::to_string(i + 1);

        std::stringstream ss;
        ss << currentOptionIndex << ". [Relic] " << item.name << " - " << item.price << "G";
        if (!item.canAfford) {
            ss << " (Unaffordable)";
        }
        item.displayString = ss.str();

        shopDisplayItems_.push_back(item);
        options_.push_back(item.displayString);
        optionInputs_.push_back(item.originalInputString);
        currentOptionIndex++;
    }
    
    ShopItemDisplay leaveItem;
    leaveItem.name = "Leave Shop";
    leaveItem.displayString = std::to_string(currentOptionIndex) + ". Leave Shop";
    leaveItem.originalInputString = "leave";
    leaveItem.price = 0;
    leaveItem.canAfford = true; 
    shopDisplayItems_.push_back(leaveItem);
    options_.push_back(leaveItem.displayString);
    optionInputs_.push_back(leaveItem.originalInputString);

    if (options_.size() == 1) {
        selectedIndex_ = 0;
    }
}

void GraphicalUI::draw() {
    LOG_DEBUG("graphical_ui", "draw() called; screenType=" + std::to_string(static_cast<int>(screenType_)) + ", overlay=" + (isShowingRewardsOverlay_ ? "true" : "false"));
    window_.clear(sf::Color::Black);
    sf::Vector2u winSize = window_.getSize();
    float winW = static_cast<float>(winSize.x);
    float winH = static_cast<float>(winSize.y);

    // --- Overlay Drawing --- (PRIORITY 1)
    if (currentOverlay_ != OverlayType::None) {
        // Semi-transparent background for the overlay to dim the underlying screen
        sf::RectangleShape overlayBg(sf::Vector2f(winW, winH));
        overlayBg.setFillColor(sf::Color(0, 0, 0, 180)); // Dark, semi-transparent
        window_.draw(overlayBg);

        // Draw Overlay Title (e.g., "EVENT RESULT", "MESSAGE")
        sf::Text overlayTitle(overlayTitleText_, font_, 48);
        sf::FloatRect overlayTitleBounds = overlayTitle.getLocalBounds();
        overlayTitle.setOrigin(overlayTitleBounds.left + overlayTitleBounds.width / 2.0f, 
                               overlayTitleBounds.top + overlayTitleBounds.height / 2.0f);
        overlayTitle.setPosition(winW / 2.0f, winH * 0.2f);
        overlayTitle.setFillColor(sf::Color::White);
        window_.draw(overlayTitle);

        // Word wrap and draw Overlay Message
        std::string fullMessage = overlayMessageText_;
        const unsigned int overlayCharSize = 28;
        float availableWidth = winW * 0.8f; // 80% of window width for text
        float currentY = winH * 0.3f;
        const float lineSpacing = 8.f; 

        std::vector<std::string> allWrappedLines;
        std::string segment;
        std::istringstream fullMessageStream(fullMessage);
        
        sf::Text testTextHelper; // Used for measuring word wrap
        testTextHelper.setFont(font_);
        testTextHelper.setCharacterSize(overlayCharSize);

        while (std::getline(fullMessageStream, segment, '\n')) {
            if (segment.empty() && !allWrappedLines.empty()) {
                allWrappedLines.push_back(""); 
                continue;
            }
            if (!segment.empty() || !allWrappedLines.empty() || fullMessageStream.peek() != EOF) {
                std::string currentWordWrapLine;
                std::istringstream segmentStream(segment);
                std::string word;
                bool firstWordInSegmentLine = true;

                while (segmentStream >> word) {
                    std::string tempLine = currentWordWrapLine;
                    if (!firstWordInSegmentLine) {
                        tempLine += " ";
                    }
                    tempLine += word;

                    testTextHelper.setString(tempLine);
                    if (testTextHelper.getLocalBounds().width > availableWidth) {
                        if (!currentWordWrapLine.empty()) {
                            allWrappedLines.push_back(currentWordWrapLine);
                        }
                        currentWordWrapLine = word;
                        firstWordInSegmentLine = true;
                    } else {
                        if (!firstWordInSegmentLine) {
                            currentWordWrapLine += " ";
                        }
                        currentWordWrapLine += word;
                        firstWordInSegmentLine = false;
                    }
                }
                if (!currentWordWrapLine.empty()) {
                    allWrappedLines.push_back(currentWordWrapLine);
                }
            }
        }

        // Calculate total height and starting Y to center the block of text
        float totalMessageHeight = 0.f;
        sf::Text tempLineForHeight("", font_, overlayCharSize);

        if (!allWrappedLines.empty()) {
            tempLineForHeight.setString(allWrappedLines[0]);
            float singleLineHeight = tempLineForHeight.getLocalBounds().height * 1.5f;
            float blockHeight = allWrappedLines.size() * singleLineHeight;
            currentY = (winH - blockHeight) / 2.0f;
            if (currentY < winH * 0.3f) currentY = winH * 0.3f;
            
            // Calculate actual totalMessageHeight based on wrapped lines and spacing
            totalMessageHeight = (allWrappedLines.size() * (tempLineForHeight.getGlobalBounds().height + lineSpacing)) - lineSpacing;
        }


        // Start Y for the first line of the message block, aiming to center the block vertically somewhat
        float currentTextY = overlayTitle.getPosition().y + overlayTitle.getGlobalBounds().height / 1.5f + 30.f;
        
        // If trying to center the whole text block based on calculated totalMessageHeight:
        if (totalMessageHeight > 0.f) {
             float centeredBlockStartY = winH / 2.0f - totalMessageHeight / 2.0f;
             currentTextY = std::max(centeredBlockStartY, overlayTitle.getPosition().y + overlayTitle.getGlobalBounds().height + 20.f);
        }


        for (const std::string& lineContent : allWrappedLines) {
            sf::Text lineText(lineContent, font_, overlayCharSize);
            sf::FloatRect lineBounds = lineText.getLocalBounds();
            // Set origin to center of the line for horizontal centering, and top for vertical alignment
            lineText.setOrigin(lineBounds.left + lineBounds.width / 2.0f, lineBounds.top);
            lineText.setPosition(winW / 2.0f, currentTextY);
            lineText.setFillColor(sf::Color::White);
            window_.draw(lineText);
            currentTextY += lineText.getGlobalBounds().height + lineSpacing;
        }
        
        return;
    }

    // --- Rewards Overlay Drawing --- (PRIORITY 2)
    if (isShowingRewardsOverlay_) {
        // Draw the main rewards title
        sf::Text rewardsTitleText(title_, font_, 48);
        sf::FloatRect titleBounds = rewardsTitleText.getLocalBounds();
        rewardsTitleText.setOrigin(titleBounds.left + titleBounds.width / 2.0f, titleBounds.top + titleBounds.height / 2.0f);
        rewardsTitleText.setPosition(winW / 2.0f, 100.f);
        rewardsTitleText.setFillColor(sf::Color::White);
        window_.draw(rewardsTitleText);

        // Draw "Gold Earned" text, centered
        std::string goldString = "Gold Earned: " + std::to_string(rewardsGoldValue_) + "G";
        sf::Text goldEarnedText(goldString, font_, 32);
        sf::FloatRect goldTextBounds = goldEarnedText.getLocalBounds();
        goldEarnedText.setOrigin(goldTextBounds.left + goldTextBounds.width / 2.0f, goldTextBounds.top + goldTextBounds.height / 2.0f);
        goldEarnedText.setPosition(winW / 2.0f, rewardsTitleText.getPosition().y + titleBounds.height + 30.f);
        goldEarnedText.setFillColor(sf::Color::White);
        window_.draw(goldEarnedText);

        sf::Text rewardsMsgText(message_, font_, 24);
        sf::FloatRect msgBounds = rewardsMsgText.getLocalBounds();        
        rewardsMsgText.setOrigin(msgBounds.left + msgBounds.width / 2.0f, msgBounds.top); 
        rewardsMsgText.setPosition(winW / 2.0f, goldEarnedText.getPosition().y + goldTextBounds.height + 30.f);
        rewardsMsgText.setFillColor(sf::Color::White);
        window_.draw(rewardsMsgText);
        
        return; 
    }

    // Draw title centered horizontally (unless it's the map screen, etc.)
    if (screenType_ != ScreenType::Map) {
        sf::Text textTitle(title_, font_, 48);
        sf::FloatRect titleBounds = textTitle.getLocalBounds();
        textTitle.setOrigin(titleBounds.left + titleBounds.width / 2.0f,
                            titleBounds.top + titleBounds.height / 2.0f);
        textTitle.setPosition(winW / 2.0f, 50.f);
        textTitle.setFillColor(sf::Color::White);
        window_.draw(textTitle);
    }

    // MAP SCREEN: Vertical layout, 4 layers, starting with choices
    if (screenType_ == ScreenType::Map) {
        window_.clear(sf::Color(20, 20, 20)); // Dark background for map

        sf::Text mapTitleText(title_, font_, 48);
        sf::FloatRect mapTitleBounds = mapTitleText.getLocalBounds();
        mapTitleText.setOrigin(mapTitleBounds.left + mapTitleBounds.width / 2.0f, mapTitleBounds.top + mapTitleBounds.height / 2.0f);
        mapTitleText.setPosition(winW / 2.0f, 30.f); 
        mapTitleText.setFillColor(sf::Color::White);
        window_.draw(mapTitleText);

        auto getRoomLetterLambda = [&](RoomType t) -> char {
            switch (t) {
                case RoomType::MONSTER:  return 'M';
                case RoomType::ELITE:    return 'E'; 
                case RoomType::BOSS:     return 'B';
                case RoomType::REST:     return 'R';
                case RoomType::EVENT:    return '?'; 
                case RoomType::SHOP:     return 'S';
                case RoomType::TREASURE: return 'T';
                default:                 return 'U'; 
            }
        };

        std::map<int, sf::Vector2f> nodePositions;
        std::map<int, int> roomDisplayLayer;

        auto drawNodeLambda = 
            [&](int roomId, sf::Vector2f pos, float radius, sf::Color circleFill, sf::Color outlineColor, float outlineThickness, bool isSelectedChoice = false) {
            if (!mapAllRooms_.count(roomId)) return;
            RoomType type = mapAllRooms_.at(roomId).type;
            
            sf::CircleShape circle(radius);
            circle.setOrigin(radius, radius);
            circle.setPosition(pos);
            circle.setFillColor(circleFill);
            circle.setOutlineColor(outlineColor);
            circle.setOutlineThickness(outlineThickness);
            if (isSelectedChoice) {
                 circle.setOutlineColor(sf::Color::Yellow);
                 circle.setOutlineThickness(outlineThickness + 2.f);
            }
            window_.draw(circle);

            char letterChar = getRoomLetterLambda(type);
            unsigned int charSize = static_cast<unsigned int>(radius * 1.1f); 
            if (charSize < 12) charSize = 12; 
            sf::Text letterText(std::string(1, letterChar), font_, charSize); 
            letterText.setFillColor(sf::Color::Black);
            if (circleFill == sf::Color::Black || circleFill == sf::Color(20,20,20)) letterText.setFillColor(sf::Color::White);

            sf::FloatRect textBounds = letterText.getLocalBounds();
            letterText.setOrigin(textBounds.left + textBounds.width / 2.0f,
                                 textBounds.top + textBounds.height / 2.0f - (charSize * 0.1f)); 
            letterText.setPosition(pos);
            window_.draw(letterText);

            nodePositions[roomId] = pos;
        };
        
        auto drawLineConnectorLambda = [&](sf::Vector2f p1, sf::Vector2f p2, sf::Color color = sf::Color(100,100,100), float thickness = 2.f) {
            sf::RectangleShape line(sf::Vector2f(std::hypot(p2.x - p1.x, p2.y - p1.y), thickness));
            line.setPosition(p1);
            line.setFillColor(color);
            line.setRotation(atan2(p2.y - p1.y, p2.x - p1.x) * 180.f / 3.14159265f);
            window_.draw(line);
        };

        const int MAX_DISPLAY_LAYERS = 4;
        std::vector<std::vector<int>> layers(MAX_DISPLAY_LAYERS);
        std::map<int, int> roomDepthForBFS;
        std::set<int> visitedRoomsForLayout;
        std::queue<std::pair<int, int>> q;

        // Initialize queue with available rooms (these will be layer 0 for display)
        for (int roomId : mapAvailableRooms_) {
            if (mapAllRooms_.count(roomId) && visitedRoomsForLayout.find(roomId) == visitedRoomsForLayout.end()) {
                q.push({roomId, 0});
                visitedRoomsForLayout.insert(roomId);
                roomDepthForBFS[roomId] = 0;
            }
        }

        while(!q.empty()){
            std::pair<int,int> current_pair = q.front();
            q.pop();
            int u_id = current_pair.first;
            int d = current_pair.second;

            if (d >= MAX_DISPLAY_LAYERS) continue; 
            layers[d].push_back(u_id);
            roomDisplayLayer[u_id] = d;

            if (d < MAX_DISPLAY_LAYERS - 1) { 
                if (mapAllRooms_.count(u_id)) {
                    for (int v_id : mapAllRooms_.at(u_id).nextRooms) {
                        if (!mapAllRooms_.count(v_id)) continue;
                        if (visitedRoomsForLayout.find(v_id) == visitedRoomsForLayout.end()) {
                            visitedRoomsForLayout.insert(v_id);
                            roomDepthForBFS[v_id] = d + 1;
                            q.push({v_id, d + 1});
                        }
                    }
                }
            }
        }
        
        // Y positions for layers (bottom up), leaving space for title. Layer 0 is closest to player (bottom).
        float y_coords[] = {winH * 0.80f, winH * 0.60f, winH * 0.40f, winH * 0.20f}; 
        float layer_radii[] = {26.f, 22.f, 18.f, 16.f}; 
        float map_area_width = winW * 0.7f; 
        sf::Vector2f playerImplicitPosition(winW / 2.0f, winH * 0.93f);

        for (int d = 0; d < MAX_DISPLAY_LAYERS; ++d) {
            if (layers[d].empty()) continue;
            float current_y = y_coords[d];
            float num_nodes_in_layer = static_cast<float>(layers[d].size());
            
            if (d == 0) {
                std::vector<int> sorted_layer0_rooms;
                for (int ar_id : mapAvailableRooms_) {
                    if (std::find(layers[d].begin(), layers[d].end(), ar_id) != layers[d].end()) {
                        sorted_layer0_rooms.push_back(ar_id);
                    }
                }
                for (int l_id : layers[d]) {
                    if (std::find(sorted_layer0_rooms.begin(), sorted_layer0_rooms.end(), l_id) == sorted_layer0_rooms.end()) {
                         sorted_layer0_rooms.push_back(l_id);
                    }
                }
                layers[d] = sorted_layer0_rooms;
            } else {
                 std::sort(layers[d].begin(), layers[d].end()); 
            }

            for (size_t i = 0; i < layers[d].size(); ++i) {
                int roomId = layers[d][i];
                float x_pos;
                if (num_nodes_in_layer == 1) {
                    x_pos = winW / 2.0f;
                } else {
                    float base_x = (winW - map_area_width) / 2.0f; 
                    x_pos = base_x + (map_area_width / (num_nodes_in_layer -1)) * i ;
                }
                sf::Vector2f node_pos(x_pos, current_y);
                nodePositions[roomId] = node_pos;
            }
        }

        // Draw ALL CONNECTORS FIRST (lines behind nodes)
        // 1. From implicit player position to Layer 0 (mapAvailableRooms_)
        if (!layers[0].empty()){
            for (size_t i=0; i < mapAvailableRooms_.size(); ++i) { 
                int roomId = mapAvailableRooms_[i];
                if (nodePositions.count(roomId)) {
                    bool isSelectedPath = (i == selectedIndex_);
                    drawLineConnectorLambda(playerImplicitPosition, nodePositions.at(roomId),
                                       isSelectedPath ? sf::Color::Yellow : sf::Color(70,70,70),
                                       isSelectedPath ? 3.f : 1.5f);
                }
            }
        }

        // 2. Between displayed layers (L0->L1, L1->L2, L2->L3)
        for (int d = 0; d < MAX_DISPLAY_LAYERS - 1; ++d) {
            for (int u_id : layers[d]) {
                if (!mapAllRooms_.count(u_id) || !nodePositions.count(u_id)) continue;
                sf::Vector2f u_pos = nodePositions.at(u_id);
                for (int v_id : mapAllRooms_.at(u_id).nextRooms) {
                    if (nodePositions.count(v_id) && roomDisplayLayer.count(v_id) && roomDisplayLayer.at(v_id) == d + 1) {
                        bool pathFromSelectedRoot = false;
                        if (d==0) {
                             auto it = std::find(mapAvailableRooms_.begin(), mapAvailableRooms_.end(), u_id);
                             if (it != mapAvailableRooms_.end() && (static_cast<size_t>(std::distance(mapAvailableRooms_.begin(), it)) == selectedIndex_)) {
                                 pathFromSelectedRoot = true;
                             }
                        }
                        drawLineConnectorLambda(u_pos, nodePositions.at(v_id), 
                                           pathFromSelectedRoot ? sf::Color(200,200,0,180) : sf::Color(80,80,80),
                                           pathFromSelectedRoot ? 2.5f : 1.5f);
                    }
                }
            }
        }
        
        // 3. From Layer 3 (top layer) to off-screen
        if (MAX_DISPLAY_LAYERS > 0 && !layers[MAX_DISPLAY_LAYERS-1].empty()){
            int top_layer_idx = MAX_DISPLAY_LAYERS -1;
            for (int u_id : layers[top_layer_idx]) {
                 if (!mapAllRooms_.count(u_id) || !nodePositions.count(u_id)) continue;
                 sf::Vector2f u_pos = nodePositions.at(u_id);
                 for (int v_id : mapAllRooms_.at(u_id).nextRooms) {
                     if (visitedRoomsForLayout.find(v_id) == visitedRoomsForLayout.end() || roomDisplayLayer.find(v_id) == roomDisplayLayer.end() || roomDisplayLayer.at(v_id) >= MAX_DISPLAY_LAYERS) {
                        sf::Vector2f offscreen_target(u_pos.x, u_pos.y - 40.f); 
                        bool pathFromSelected = false; 
                        drawLineConnectorLambda(u_pos, offscreen_target, 
                                           pathFromSelected ? sf::Color(180,180,0,150) : sf::Color(70,70,70), 
                                           1.5f);
                     }
                 }
            }
        }

        // Draw ALL NODES (and their numbers) on top of lines
        for (int d = 0; d < MAX_DISPLAY_LAYERS; ++d) {
            if (layers[d].empty()) continue;

            for (size_t i = 0; i < layers[d].size(); ++i) {
                int roomId = layers[d][i];
                if (!nodePositions.count(roomId)) continue;
                sf::Vector2f node_pos = nodePositions.at(roomId);
                
                bool isSelected = false;
                if (d == 0) { 
                    auto it = std::find(mapAvailableRooms_.begin(), mapAvailableRooms_.end(), roomId);
                    if (it != mapAvailableRooms_.end()){
                         size_t choice_idx = std::distance(mapAvailableRooms_.begin(), it);
                         if (choice_idx == selectedIndex_) {
                             isSelected = true;
                         }
                    }
                }
                
                drawNodeLambda(roomId, node_pos, layer_radii[d], sf::Color::White, 
                               isSelected ? sf::Color::Yellow : sf::Color(60,60,60), 
                               isSelected ? 4.f : 2.f, 
                               isSelected);

                if (d == 0) { 
                    auto it = std::find(mapAvailableRooms_.begin(), mapAvailableRooms_.end(), roomId);
                     if (it != mapAvailableRooms_.end()){
                        size_t choice_idx = std::distance(mapAvailableRooms_.begin(), it);
                        sf::Text idxText(std::to_string(choice_idx + 1), font_, 20); 
                        idxText.setFillColor(isSelected ? sf::Color::Yellow : sf::Color(200,200,200));
                        idxText.setStyle(sf::Text::Bold);
                        idxText.setPosition(node_pos.x + layer_radii[d] * 0.8f, node_pos.y - layer_radii[d] * 1.5f);
                        window_.draw(idxText);
                    }
                }
            }
        }

        // Draw Legend
        float legendX = winW - 170.f; 
        float legendY = 70.f;
        sf::Text legendTitle("Legend:", font_, 20);
        legendTitle.setFillColor(sf::Color::White);
        legendTitle.setPosition(legendX, legendY);
        window_.draw(legendTitle);
        legendY += 30.f;

        std::vector<std::pair<RoomType, std::string>> legendItems = {
            {RoomType::MONSTER, "Monster"},
            {RoomType::ELITE,   "Elite"},
            {RoomType::EVENT,   "Event"},
            {RoomType::REST,    "Rest"},
            {RoomType::SHOP,    "Shop"},
            {RoomType::TREASURE,"Treasure"},
            {RoomType::BOSS,    "Boss"}
        };

        for (const auto& item : legendItems) {
            // Legend Icon Circle
            sf::CircleShape legendIcon(8.f);
            legendIcon.setFillColor(sf::Color::White); 
            legendIcon.setOrigin(legendIcon.getRadius(), legendIcon.getRadius());
            sf::Vector2f iconCenterPos(legendX + 10.f, legendY + 10.f);
            legendIcon.setPosition(iconCenterPos);
            window_.draw(legendIcon);

            // Legend Icon Letter
            sf::Text legendIconText(std::string(1, getRoomLetterLambda(item.first)), font_, 9);
            legendIconText.setFillColor(sf::Color::Black);
            sf::FloatRect lib = legendIconText.getLocalBounds();
            legendIconText.setPosition(iconCenterPos.x - lib.width / 2.f - lib.left, 
                                       iconCenterPos.y - lib.height / 2.f - lib.top + 1.0f);
            window_.draw(legendIconText);

            // Legend Entry Text (e.g., "Monster")
            sf::Text legendEntry(item.second, font_, 16);
            legendEntry.setFillColor(sf::Color::White);
            sf::FloatRect entryBounds = legendEntry.getLocalBounds();
            legendEntry.setOrigin(entryBounds.left, entryBounds.top + entryBounds.height / 2.f);

            legendEntry.setPosition(iconCenterPos.x + legendIcon.getRadius() + 8.f, iconCenterPos.y);
            window_.draw(legendEntry);
            legendY += 25.f;
        }

        // Draw Instructions
        float instructionY = winH - 25.f;
        float currentX = 0;
        unsigned int instFontSize = 18;

        sf::Text navText("Navigate: ", font_, instFontSize);
        sf::Text slashText(" / ", font_, instFontSize);
        sf::Text confirmText("  |  Confirm: Enter  |  Back: Esc", font_, instFontSize);

        float arrowHeight = instFontSize * 0.8f;
        float arrowWidth = arrowHeight * 0.7f;

        // Calculate total width for centering
        float totalWidth = navText.getLocalBounds().width + navText.getLocalBounds().left;
        totalWidth += arrowWidth + 2; // Left arrow + padding
        totalWidth += slashText.getLocalBounds().width + slashText.getLocalBounds().left;
        totalWidth += arrowWidth + 2; // Right arrow + padding
        totalWidth += confirmText.getLocalBounds().width + confirmText.getLocalBounds().left;
        
        currentX = (winW - totalWidth) / 2.0f; // Starting X for the whole line

        // Draw "Navigate: "
        navText.setPosition(currentX, instructionY - navText.getLocalBounds().height / 2.f - navText.getLocalBounds().top);
        navText.setFillColor(sf::Color::White);
        window_.draw(navText);
        currentX += navText.getLocalBounds().width + navText.getLocalBounds().left + 2;

        // Draw Left Arrow Shape
        sf::ConvexShape leftArrow;
        leftArrow.setPointCount(3);
        leftArrow.setPoint(0, sf::Vector2f(arrowWidth, 0));
        leftArrow.setPoint(1, sf::Vector2f(arrowWidth, arrowHeight));
        leftArrow.setPoint(2, sf::Vector2f(0, arrowHeight / 2.f));
        leftArrow.setFillColor(sf::Color::White);
        leftArrow.setPosition(currentX, instructionY - arrowHeight / 2.f);
        window_.draw(leftArrow);
        currentX += arrowWidth + 2;

        // Draw " / "
        slashText.setPosition(currentX, instructionY - slashText.getLocalBounds().height / 2.f - slashText.getLocalBounds().top);
        slashText.setFillColor(sf::Color::White);
        window_.draw(slashText);
        currentX += slashText.getLocalBounds().width + slashText.getLocalBounds().left + 2;

        // Draw Right Arrow Shape
        sf::ConvexShape rightArrow;
        rightArrow.setPointCount(3);
        rightArrow.setPoint(0, sf::Vector2f(0, 0)); // Tip pointing right
        rightArrow.setPoint(1, sf::Vector2f(0, arrowHeight));
        rightArrow.setPoint(2, sf::Vector2f(arrowWidth, arrowHeight / 2.f));
        rightArrow.setFillColor(sf::Color::White);
        rightArrow.setPosition(currentX, instructionY - arrowHeight / 2.f);
        window_.draw(rightArrow);
        currentX += arrowWidth + 2;

        // Draw rest of instructions
        confirmText.setPosition(currentX, instructionY - confirmText.getLocalBounds().height / 2.f - confirmText.getLocalBounds().top);
        confirmText.setFillColor(sf::Color::White);
        window_.draw(confirmText);

        return; 
    }

    // Combat Screen Drawing
    if (screenType_ == ScreenType::Combat) {
        if (!combat_) {
            sf::Text errorText(message_, font_, 32);
            sf::FloatRect errorBounds = errorText.getLocalBounds();
            errorText.setOrigin(errorBounds.left + errorBounds.width / 2.0f, errorBounds.top + errorBounds.height / 2.0f);
            errorText.setPosition(winW / 2.0f, winH / 2.0f);
            errorText.setFillColor(sf::Color::Red);
            window_.draw(errorText);
            return;
        }

        // Define areas for player and enemies - these are needed for both info and options layout
        float topMargin = 80.f;
        float infoAreaHeight = 220.f;
        float infoAreaWidth = winW * 0.35f;

        sf::FloatRect playerArea(30.f, topMargin, infoAreaWidth, infoAreaHeight);
        
        // Draw Player Info
        drawPlayerInfoGfx(window_, playerArea);

        // Draw Enemies (layout for multiple enemies)
        size_t aliveEnemies = 0;
        for (size_t i = 0; i < combat_->getEnemyCount(); ++i) {
            if (combat_->getEnemy(i) && combat_->getEnemy(i)->isAlive()) {
                aliveEnemies++;
            }
        }

        if (aliveEnemies > 0) {
            float enemyAreaTotalWidth = winW * 0.5f; 
            float enemyAreaIndividualWidth = (aliveEnemies == 1) ? infoAreaWidth : (enemyAreaTotalWidth / aliveEnemies) - 20.f; 
            float enemyStartX = winW - 30.f - ((aliveEnemies == 1) ? infoAreaWidth : enemyAreaTotalWidth) ;
            
            size_t drawnEnemyIndex = 0;
            for (size_t i = 0; i < combat_->getEnemyCount(); ++i) {
                const Enemy* enemy = combat_->getEnemy(i);
                if (enemy && enemy->isAlive()) {
                    sf::FloatRect enemyArea(enemyStartX + drawnEnemyIndex * (enemyAreaIndividualWidth + 20.f), 
                                            topMargin, 
                                            enemyAreaIndividualWidth, 
                                            infoAreaHeight);
                    drawEnemyInfoGfx(window_, enemy, enemyArea);
                    drawnEnemyIndex++;
                }
            }
        }
        
        if (!combat_->isPlayerTurn()) {
            sf::Text enemyTurnText("Enemies are taking their turns...", font_, 28);
            sf::FloatRect etBounds = enemyTurnText.getLocalBounds();
            enemyTurnText.setOrigin(etBounds.left + etBounds.width / 2.0f, etBounds.top + etBounds.height / 2.0f);
            enemyTurnText.setPosition(winW / 2.0f, winH / 2.0f); 
            enemyTurnText.setFillColor(sf::Color::White);
            window_.draw(enemyTurnText);
        } else {
            float optionStartY = topMargin + infoAreaHeight + 40.f; 
            float optionSpacingY = 45.f; 
            unsigned int cardCharSize = 20;
            unsigned int descCharSize = 15;

            const auto* player = combat_->getPlayer();
            if (!player) return;
            const auto& hand = player->getHand();

            for (size_t i = 0; i < options_.size(); ++i) {
                std::string displayLabel = options_[i];
                std::string currentOptionInput = (i < optionInputs_.size()) ? optionInputs_[i] : "";

                if (i < hand.size()) { 
                    if (!currentOptionInput.empty() && std::all_of(currentOptionInput.begin(), currentOptionInput.end(), ::isdigit)) {
                         displayLabel = currentOptionInput + ". " + options_[i];
                    }
                } else if (options_[i] == "End Turn") {
                    displayLabel = "[E] " + options_[i];
                }

                sf::Text opt(displayLabel, font_, cardCharSize);
                sf::FloatRect optBounds = opt.getLocalBounds();
                opt.setOrigin(optBounds.left + optBounds.width / 2.0f, optBounds.top + optBounds.height / 2.0f);
                float currentOptionY = optionStartY + i * optionSpacingY;
                opt.setPosition(winW / 2.0f, currentOptionY);
                opt.setFillColor(i == selectedIndex_ ? sf::Color::Yellow : sf::Color::White);
                window_.draw(opt);

                if (i < hand.size()) { 
                    const Card* card = hand[i].get(); 
                    if (card) {
                        sf::Text descText("  " + card->getDescription(), font_, descCharSize);
                        descText.setFillColor(i == selectedIndex_ ? sf::Color::Yellow : sf::Color(200, 200, 200));
                        sf::FloatRect descOptBounds = descText.getLocalBounds();
                        descText.setOrigin(descOptBounds.left + descOptBounds.width / 2.0f, descOptBounds.top + descOptBounds.height / 2.0f);
                        descText.setPosition(winW / 2.0f, currentOptionY + cardCharSize * 0.8f );
                        window_.draw(descText);
                    }
                }
            }
        }
    } else if (screenType_ == ScreenType::EnemySelection) {
        // Draw enemy selection menu
        if (!combat_) {
            sf::Text errorText("ERROR: No combat data available", font_, 32);
            sf::FloatRect errorBounds = errorText.getLocalBounds();
            errorText.setOrigin(errorBounds.left + errorBounds.width / 2.0f, errorBounds.top + errorBounds.height / 2.0f);
            errorText.setPosition(winW / 2.0f, winH / 2.0f);
            errorText.setFillColor(sf::Color::Red);
            window_.draw(errorText);
            return;
        }
        
        float optionStartY = 120.f;
        float optionSpacingY = 45.f;
        
        // Draw enemy stats for each option
        for (size_t i = 0; i < combat_->getEnemyCount(); ++i) {
            Enemy* enemy = combat_->getEnemy(i);
            if (enemy && enemy->isAlive()) {
                size_t optionIndex = 0;
                for (size_t j = 0; j < options_.size(); ++j) {
                    if (options_[j] == enemy->getName()) {
                        optionIndex = j;
                        break;
                    }
                }
                
                sf::FloatRect enemyArea(winW / 4.0f, 
                                      optionStartY + optionIndex * optionSpacingY, 
                                      winW / 2.0f, 
                                      150.f);
                                      
                // Draw highlight box if this is the selected enemy
                if (optionIndex == selectedIndex_) {
                    sf::RectangleShape highlight(sf::Vector2f(enemyArea.width + 10.f, enemyArea.height + 10.f));
                    highlight.setPosition(enemyArea.left - 5.f, enemyArea.top - 5.f);
                    highlight.setFillColor(sf::Color(0, 0, 0, 0));
                    highlight.setOutlineColor(sf::Color::Yellow);
                    highlight.setOutlineThickness(2.f);
                    window_.draw(highlight);
                }
                
                drawEnemyInfoGfx(window_, enemy, enemyArea);
            }
        }
        
        // Draw cancel option
        if (options_.size() > combat_->getEnemyCount()) {
            sf::Text cancelText(options_[options_.size() - 1], font_, 32);
            sf::FloatRect cancelBounds = cancelText.getLocalBounds();
            cancelText.setOrigin(cancelBounds.left + cancelBounds.width / 2.0f, cancelBounds.top + cancelBounds.height / 2.0f);
            float y = optionStartY + (options_.size() - 1) * optionSpacingY;
            cancelText.setPosition(winW / 2.0f, y);
            cancelText.setFillColor(selectedIndex_ == options_.size() - 1 ? sf::Color::Yellow : sf::Color::White);
            window_.draw(cancelText);
        }
        
    } else if (screenType_ == ScreenType::Message ||
        screenType_ == ScreenType::EventResult ||
        screenType_ == ScreenType::GameOver) {
        sf::Text msg(message_, font_, 32);
        sf::FloatRect msgBounds = msg.getLocalBounds();
        msg.setOrigin(msgBounds.left + msgBounds.width / 2.0f, msgBounds.top + msgBounds.height / 2.0f);
        msg.setPosition(winW / 2.0f, winH / 2.0f);
        msg.setFillColor(sf::Color::White);
        window_.draw(msg);
    } else if (screenType_ == ScreenType::Event || screenType_ == ScreenType::Rest) {
        // Draw options for events or rest sites
        float optionStartY = 200.f;
        float optionSpacingY = 45.f;
        
        for (size_t i = 0; i < options_.size(); ++i) {
            std::string displayText = std::to_string(i + 1) + ". " + options_[i];
            sf::Text opt(displayText, font_, 24);
            sf::FloatRect optBounds = opt.getLocalBounds();
            opt.setOrigin(optBounds.left + optBounds.width / 2.0f, optBounds.top + optBounds.height / 2.0f);
            opt.setPosition(winW / 2.0f, optionStartY + i * optionSpacingY);
            opt.setFillColor(i == selectedIndex_ ? sf::Color::Yellow : sf::Color::White);
            window_.draw(opt);
        }
    } else if (screenType_ == ScreenType::Shop) {
        float optionStartY = 120.f;
        float optionSpacingY = 35.f;
        unsigned int itemCharSize = 20;

        for (size_t i = 0; i < options_.size(); ++i) {
            sf::Text opt(options_[i], font_, itemCharSize);
            opt.setPosition(50.f, optionStartY + i * optionSpacingY);
            
            sf::Color itemColor = sf::Color::White;
            if (i < shopDisplayItems_.size() && !shopDisplayItems_[i].canAfford) {
                itemColor = sf::Color(150, 150, 150); // Grey out unaffordable items
            }
            if (i == selectedIndex_) {
                itemColor = sf::Color::Yellow; // Highlight selected
            }
            opt.setFillColor(itemColor);
            window_.draw(opt);
        }
    } else if (screenType_ == ScreenType::CardView) {
        // Display a single card
        if (cardToDisplay_) {
            float cardWidth = 300.f;
            float cardHeight = 400.f;
            float padding = 15.f;
            
            // Card background
            sf::RectangleShape cardBg(sf::Vector2f(cardWidth, cardHeight));
            cardBg.setPosition((winW - cardWidth) / 2.f, (winH - cardHeight) / 2.f + 20.f);
            
            // Set card background color based on card type
            sf::Color bgColor;
            switch (cardToDisplay_->getType()) {
                case CardType::ATTACK: bgColor = sf::Color(180, 60, 60, 220); break;  // Red for attack
                case CardType::SKILL: bgColor = sf::Color(60, 120, 180, 220); break;  // Blue for skill
                case CardType::POWER: bgColor = sf::Color(160, 60, 180, 220); break;  // Purple for power
                case CardType::STATUS: bgColor = sf::Color(150, 150, 100, 220); break; // Yellow-ish for status
                case CardType::CURSE: bgColor = sf::Color(100, 60, 100, 220); break;  // Darker purple for curse
                default: bgColor = sf::Color(120, 120, 120, 220); break;  // Gray for unknown
            }
            
            cardBg.setFillColor(bgColor);
            cardBg.setOutlineColor(isCardSelected_ ? sf::Color::Yellow : sf::Color(200, 200, 200));
            cardBg.setOutlineThickness(isCardSelected_ ? 3.f : 1.f);
            window_.draw(cardBg);
            
            sf::FloatRect cardRect = cardBg.getGlobalBounds();
            float currentY = cardRect.top + padding;
            
            // Card name
            sf::Text nameText;
            nameText.setFont(font_);
            nameText.setCharacterSize(24);
            nameText.setFillColor(sf::Color::White);
            
            std::string nameStr = cardToDisplay_->getName();
            if (cardToDisplay_->isUpgraded()) nameStr += "+";
            if (showCardEnergyCost_) {
                nameStr = "[" + std::to_string(cardToDisplay_->getCost()) + "] " + nameStr;
            }
            
            nameText.setString(nameStr);
            sf::FloatRect nameBounds = nameText.getLocalBounds();
            nameText.setOrigin(nameBounds.left + nameBounds.width / 2.f, nameBounds.top);
            nameText.setPosition(cardRect.left + cardRect.width / 2.f, currentY);
            window_.draw(nameText);
            currentY += nameBounds.height + padding;
            
            // Card type
            sf::Text typeText;
            typeText.setFont(font_);
            typeText.setCharacterSize(18);
            typeText.setFillColor(sf::Color(230, 230, 230));
            typeText.setString(getCardTypeStringGfx(cardToDisplay_->getType()));
            sf::FloatRect typeBounds = typeText.getLocalBounds();
            typeText.setOrigin(typeBounds.left + typeBounds.width / 2.f, typeBounds.top);
            typeText.setPosition(cardRect.left + cardRect.width / 2.f, currentY);
            window_.draw(typeText);
            currentY += typeBounds.height + padding * 1.5f;
            
            // Card description
            sf::Text descText;
            descText.setFont(font_);
            descText.setCharacterSize(18);
            descText.setFillColor(sf::Color::White);
            descText.setString(cardToDisplay_->getDescription());
            
            // Handle word wrapping for description
            std::string desc = cardToDisplay_->getDescription();
            std::vector<std::string> descLines;
            
            // Word wrap algorithm
            std::string currentLine;
            std::istringstream descStream(desc);
            std::string word;
            while (descStream >> word) {
                sf::Text testText;
                testText.setFont(font_);
                testText.setCharacterSize(18);
                testText.setString(currentLine + " " + word);
                
                if (testText.getLocalBounds().width > (cardWidth - padding * 2)) {
                    if (!currentLine.empty()) {
                        descLines.push_back(currentLine);
                        currentLine = word;
                    } else {
                        descLines.push_back(word);
                        currentLine = "";
                    }
                } else {
                    if (currentLine.empty()) {
                        currentLine = word;
                    } else {
                        currentLine += " " + word;
                    }
                }
            }
            
            if (!currentLine.empty()) {
                descLines.push_back(currentLine);
            }
            
            // Draw each line of description
            for (const auto& line : descLines) {
                sf::Text lineText;
                lineText.setFont(font_);
                lineText.setCharacterSize(18);
                lineText.setFillColor(sf::Color::White);
                lineText.setString(line);
                lineText.setPosition(cardRect.left + padding, currentY);
                window_.draw(lineText);
                currentY += lineText.getLocalBounds().height + 5.f;
            }
            
            // Press to continue message
            sf::Text continueText;
            continueText.setFont(font_);
            continueText.setCharacterSize(16);
            continueText.setFillColor(sf::Color(180, 180, 180));
            continueText.setString("Press any key to continue");
            sf::FloatRect contBounds = continueText.getLocalBounds();
            continueText.setOrigin(contBounds.left + contBounds.width / 2.f, contBounds.top);
            continueText.setPosition(winW / 2.f, cardRect.top + cardRect.height + 30.f);
            window_.draw(continueText);
        }
    } else if (screenType_ == ScreenType::CardsView) {
        // Display multiple cards in a grid
        if (cardsToDisplay_.empty()) {
            sf::Text noCardsText("No cards to display", font_, 32);
            sf::FloatRect textBounds = noCardsText.getLocalBounds();
            noCardsText.setOrigin(textBounds.left + textBounds.width / 2.f, textBounds.top + textBounds.height / 2.f);
            noCardsText.setPosition(winW / 2.f, winH / 2.f);
            noCardsText.setFillColor(sf::Color::White);
            window_.draw(noCardsText);
            
            // Press to continue message
            sf::Text continueText("Press any key to continue", font_, 20);
            sf::FloatRect contBounds = continueText.getLocalBounds();
            continueText.setOrigin(contBounds.left + contBounds.width / 2.f, contBounds.top);
            continueText.setPosition(winW / 2.f, winH / 2.f + 50.f);
            continueText.setFillColor(sf::Color(180, 180, 180));
            window_.draw(continueText);
            return;
        }
        
        // Card grid layout
        float cardWidth = 220.f;
        float cardHeight = 300.f;
        float padding = 20.f;
        float startX = padding;
        float startY = 120.f; // Below title
        int cardsPerRow = std::max(1, static_cast<int>((winW - padding) / (cardWidth + padding)));
        
        for (size_t i = 0; i < cardsToDisplay_.size(); ++i) {
            const Card* card = cardsToDisplay_[i];
            if (!card) continue;
            
            int row = i / cardsPerRow;
            int col = i % cardsPerRow;
            float x = startX + col * (cardWidth + padding);
            float y = startY + row * (cardHeight + padding);
            
            // Card background
            sf::RectangleShape cardBg(sf::Vector2f(cardWidth, cardHeight));
            cardBg.setPosition(x, y);
            
            // Set card background color based on card type
            sf::Color bgColor;
            switch (card->getType()) {
                case CardType::ATTACK: bgColor = sf::Color(180, 60, 60, 220); break;  // Red for attack
                case CardType::SKILL: bgColor = sf::Color(60, 120, 180, 220); break;  // Blue for skill
                case CardType::POWER: bgColor = sf::Color(160, 60, 180, 220); break;  // Purple for power
                case CardType::STATUS: bgColor = sf::Color(150, 150, 100, 220); break; // Yellow-ish for status
                case CardType::CURSE: bgColor = sf::Color(100, 60, 100, 220); break;  // Darker purple for curse
                default: bgColor = sf::Color(120, 120, 120, 220); break;  // Gray for unknown
            }
            
            cardBg.setFillColor(bgColor);
            cardBg.setOutlineColor(i == selectedIndex_ ? sf::Color::Yellow : sf::Color(200, 200, 200));
            cardBg.setOutlineThickness(i == selectedIndex_ ? 3.f : 1.f);
            window_.draw(cardBg);
            
            // Display index if requested
            if (showCardIndices_) {
                sf::Text idxText;
                idxText.setFont(font_);
                idxText.setCharacterSize(20);
                idxText.setFillColor(sf::Color::White);
                idxText.setOutlineColor(sf::Color::Black);
                idxText.setOutlineThickness(1.f);
                idxText.setString(std::to_string(i + 1));
                // Position the index slightly above the card
                sf::FloatRect idxBounds = idxText.getLocalBounds();
                idxText.setPosition(x + 10.f, y - idxBounds.height - 5.f); 
                window_.draw(idxText);
            }
            
            // Card name
            sf::Text nameText;
            nameText.setFont(font_);
            nameText.setCharacterSize(18);
            nameText.setFillColor(sf::Color::White);
            
            std::string nameStr = card->getName();
            if (card->isUpgraded()) nameStr += "+";
            
            nameText.setString(nameStr);
            sf::FloatRect nameBounds = nameText.getLocalBounds();
            nameText.setOrigin(nameBounds.left + nameBounds.width / 2.f, nameBounds.top);
            nameText.setPosition(x + cardWidth / 2.f, y + 20.f);
            window_.draw(nameText);
            
            // Energy cost (upper left corner)
            sf::CircleShape costCircle(15.f);
            costCircle.setPosition(x + 15.f, y + 15.f);
            costCircle.setFillColor(sf::Color(30, 30, 80));
            costCircle.setOutlineColor(sf::Color(100, 100, 200));
            costCircle.setOutlineThickness(2.f);
            window_.draw(costCircle);
            
            sf::Text costText;
            costText.setFont(font_);
            costText.setCharacterSize(20);
            costText.setFillColor(sf::Color::White);
            costText.setString(std::to_string(card->getCost()));
            sf::FloatRect costBounds = costText.getLocalBounds();
            costText.setOrigin(costBounds.left + costBounds.width / 2.f, costBounds.top + costBounds.height / 2.f);
            costText.setPosition(x + 15.f + costCircle.getRadius(), y + 15.f + costCircle.getRadius());
            window_.draw(costText);
            
            // Card type
            sf::Text typeText;
            typeText.setFont(font_);
            typeText.setCharacterSize(14);
            typeText.setFillColor(sf::Color(230, 230, 230));
            typeText.setString(getCardTypeStringGfx(card->getType()));
            sf::FloatRect typeBounds = typeText.getLocalBounds();
            typeText.setOrigin(typeBounds.left + typeBounds.width / 2.f, typeBounds.top);
            typeText.setPosition(x + cardWidth / 2.f, y + 45.f);
            window_.draw(typeText);
            
            // Card description
            sf::Text descText;
            descText.setFont(font_);
            descText.setCharacterSize(14);
            descText.setFillColor(sf::Color::White);
            
            // Word wrap for description
            std::string desc = card->getDescription();
            std::vector<std::string> descLines;
            
            // Word wrap
            std::string currentLine;
            std::istringstream descStream(desc);
            std::string word;
            
            while (descStream >> word) {
                sf::Text testText;
                testText.setFont(font_);
                testText.setCharacterSize(14);
                testText.setString(currentLine + " " + word);
                
                if (testText.getLocalBounds().width > (cardWidth - 20.f)) {
                    if (!currentLine.empty()) {
                        descLines.push_back(currentLine);
                        currentLine = word;
                    } else {
                        descLines.push_back(word);
                        currentLine = "";
                    }
                } else {
                    if (currentLine.empty()) {
                        currentLine = word;
                    } else {
                        currentLine += " " + word;
                    }
                }
            }
            
            if (!currentLine.empty()) {
                descLines.push_back(currentLine);
            }
            
            // Draw each line of description
            float descY = y + 70.f;
            for (size_t j = 0; j < descLines.size() && j < 6; ++j) { // Limit to 6 lines
                sf::Text lineText;
                lineText.setFont(font_);
                lineText.setCharacterSize(14);
                lineText.setFillColor(sf::Color::White);
                lineText.setString(descLines[j]);
                sf::FloatRect lineBounds = lineText.getLocalBounds();
                lineText.setOrigin(lineBounds.left + lineBounds.width / 2.f, lineBounds.top);
                lineText.setPosition(x + cardWidth / 2.f, descY);
                window_.draw(lineText);
                descY += lineText.getLocalBounds().height + 5.f;
            }
            
            // If there are more lines than can fit, show ellipsis
            if (descLines.size() > 6) {
                sf::Text ellipsisText("...", font_, 14);
                sf::FloatRect ellipsisBounds = ellipsisText.getLocalBounds();
                ellipsisText.setOrigin(ellipsisBounds.left + ellipsisBounds.width / 2.f, ellipsisBounds.top);
                ellipsisText.setPosition(x + cardWidth / 2.f, descY);
                ellipsisText.setFillColor(sf::Color::White);
                window_.draw(ellipsisText);
            }
        }
        
        // Draw "Close" option at the bottom
        if (!options_.empty()) {
            sf::Text closeText("Close", font_, 24);
            sf::FloatRect closeBounds = closeText.getLocalBounds();
            closeText.setOrigin(closeBounds.left + closeBounds.width / 2.f, closeBounds.top + closeBounds.height / 2.f);
            closeText.setPosition(winW / 2.f, startY + ((cardsToDisplay_.size() + cardsPerRow - 1) / cardsPerRow) * (cardHeight + padding) + 40.f);
            closeText.setFillColor(selectedIndex_ == options_.size() - 1 ? sf::Color::Yellow : sf::Color::White);
            window_.draw(closeText);
        }
        
    } else if (screenType_ == ScreenType::RelicView) {
        // Display a single relic
        if (relicToDisplay_) {
            float relicWidth = 300.f;
            float relicHeight = 300.f;
            float padding = 15.f;
            
            // Relic background
            sf::RectangleShape relicBg(sf::Vector2f(relicWidth, relicHeight));
            relicBg.setPosition((winW - relicWidth) / 2.f, (winH - relicHeight) / 2.f);
            
            // Set relic background color based on rarity
            sf::Color bgColor;
            switch (relicToDisplay_->getRarity()) {
                case RelicRarity::COMMON: bgColor = sf::Color(120, 120, 120, 220); break;  // Gray for common
                case RelicRarity::UNCOMMON: bgColor = sf::Color(60, 160, 100, 220); break; // Green for uncommon
                case RelicRarity::RARE: bgColor = sf::Color(60, 100, 180, 220); break;     // Blue for rare
                case RelicRarity::BOSS: bgColor = sf::Color(180, 60, 60, 220); break;      // Red for boss
                case RelicRarity::EVENT: bgColor = sf::Color(160, 100, 60, 220); break;    // Orange for event
                case RelicRarity::SHOP: bgColor = sf::Color(160, 130, 60, 220); break;     // Gold for shop
                default: bgColor = sf::Color(120, 120, 120, 220); break;                   // Gray default
            }
            
            relicBg.setFillColor(bgColor);
            relicBg.setOutlineColor(sf::Color(200, 200, 200));
            relicBg.setOutlineThickness(2.f);
            window_.draw(relicBg);
            
            sf::FloatRect relicRect = relicBg.getGlobalBounds();
            float currentY = relicRect.top + padding;
            
            // Relic name
            sf::Text nameText;
            nameText.setFont(font_);
            nameText.setCharacterSize(24);
            nameText.setFillColor(sf::Color::White);
            nameText.setString(relicToDisplay_->getName());
            sf::FloatRect nameBounds = nameText.getLocalBounds();
            nameText.setOrigin(nameBounds.left + nameBounds.width / 2.f, nameBounds.top);
            nameText.setPosition(relicRect.left + relicRect.width / 2.f, currentY);
            window_.draw(nameText);
            currentY += nameBounds.height + padding;
            
            // Relic rarity
            std::string rarityStr;
            switch (relicToDisplay_->getRarity()) {
                case RelicRarity::COMMON: rarityStr = "Common"; break;
                case RelicRarity::UNCOMMON: rarityStr = "Uncommon"; break;
                case RelicRarity::RARE: rarityStr = "Rare"; break;
                case RelicRarity::BOSS: rarityStr = "Boss"; break;
                case RelicRarity::STARTER: rarityStr = "Starter"; break;
                case RelicRarity::EVENT: rarityStr = "Event"; break;
                case RelicRarity::SHOP: rarityStr = "Shop"; break;
                default: rarityStr = "Unknown"; break;
            }
            
            sf::Text rarityText;
            rarityText.setFont(font_);
            rarityText.setCharacterSize(18);
            rarityText.setFillColor(sf::Color(230, 230, 230));
            rarityText.setString(rarityStr);
            sf::FloatRect rarityBounds = rarityText.getLocalBounds();
            rarityText.setOrigin(rarityBounds.left + rarityBounds.width / 2.f, rarityBounds.top);
            rarityText.setPosition(relicRect.left + relicRect.width / 2.f, currentY);
            window_.draw(rarityText);
            currentY += rarityBounds.height + padding * 1.5f;
            
            // Relic description
            std::string desc = relicToDisplay_->getDescription();
            std::vector<std::string> descLines;
            
            // Word wrap algorithm
            std::string currentLine;
            std::istringstream descStream(desc);
            std::string word;
            while (descStream >> word) {
                sf::Text testText;
                testText.setFont(font_);
                testText.setCharacterSize(18);
                testText.setString(currentLine + " " + word);
                
                if (testText.getLocalBounds().width > (relicWidth - padding * 2)) {
                    if (!currentLine.empty()) {
                        descLines.push_back(currentLine);
                        currentLine = word;
                    } else {
                        descLines.push_back(word);
                        currentLine = "";
                    }
                } else {
                    if (currentLine.empty()) {
                        currentLine = word;
                    } else {
                        currentLine += " " + word;
                    }
                }
            }
            
            if (!currentLine.empty()) {
                descLines.push_back(currentLine);
            }
            
            // Draw each line of description
            for (const auto& line : descLines) {
                sf::Text lineText;
                lineText.setFont(font_);
                lineText.setCharacterSize(18);
                lineText.setFillColor(sf::Color::White);
                lineText.setString(line);
                lineText.setPosition(relicRect.left + padding, currentY);
                window_.draw(lineText);
                currentY += lineText.getLocalBounds().height + 5.f;
            }
            
            currentY += padding;
            
            // Flavor text (if any)
            if (!relicToDisplay_->getFlavorText().empty()) {
                std::string flavor = "\"" + relicToDisplay_->getFlavorText() + "\"";
                std::vector<std::string> flavorLines;
                
                // Word wrap for flavor text
                currentLine = "";
                std::istringstream flavorStream(flavor);
                while (flavorStream >> word) {
                    sf::Text testText;
                    testText.setFont(font_);
                    testText.setCharacterSize(16);
                    testText.setString(currentLine + " " + word);
                    
                    if (testText.getLocalBounds().width > (relicWidth - padding * 2)) {
                        if (!currentLine.empty()) {
                            flavorLines.push_back(currentLine);
                            currentLine = word;
                        } else {
                            flavorLines.push_back(word);
                            currentLine = "";
                        }
                    } else {
                        if (currentLine.empty()) {
                            currentLine = word;
                        } else {
                            currentLine += " " + word;
                        }
                    }
                }
                
                if (!currentLine.empty()) {
                    flavorLines.push_back(currentLine);
                }
                
                // Draw each line of flavor text
                for (const auto& line : flavorLines) {
                    sf::Text lineText;
                    lineText.setFont(font_);
                    lineText.setCharacterSize(16);
                    lineText.setFillColor(sf::Color(180, 180, 180));
                    lineText.setStyle(sf::Text::Italic);
                    lineText.setString(line);
                    sf::FloatRect lineBounds = lineText.getLocalBounds();
                    lineText.setOrigin(lineBounds.left + lineBounds.width / 2.f, lineBounds.top);
                    lineText.setPosition(relicRect.left + relicRect.width / 2.f, currentY);
                    window_.draw(lineText);
                    currentY += lineText.getLocalBounds().height + 3.f;
                }
            }
            
            // Press to continue message
            sf::Text continueText;
            continueText.setFont(font_);
            continueText.setCharacterSize(16);
            continueText.setFillColor(sf::Color(180, 180, 180));
            continueText.setString("Press any key to continue");
            sf::FloatRect contBounds = continueText.getLocalBounds();
            continueText.setOrigin(contBounds.left + contBounds.width / 2.f, contBounds.top);
            continueText.setPosition(winW / 2.f, relicRect.top + relicRect.height + 30.f);
            window_.draw(continueText);
        }
    } else if (screenType_ == ScreenType::RelicsView) {
        // Display multiple relics in a grid
        if (relicsToDisplay_.empty()) {
            sf::Text noRelicsText("No relics to display", font_, 32);
            sf::FloatRect textBounds = noRelicsText.getLocalBounds();
            noRelicsText.setOrigin(textBounds.left + textBounds.width / 2.f, textBounds.top + textBounds.height / 2.f);
            noRelicsText.setPosition(winW / 2.f, winH / 2.f);
            noRelicsText.setFillColor(sf::Color::White);
            window_.draw(noRelicsText);
            
            // Press to continue message
            sf::Text continueText("Press any key to continue", font_, 20);
            sf::FloatRect contBounds = continueText.getLocalBounds();
            continueText.setOrigin(contBounds.left + contBounds.width / 2.f, contBounds.top);
            continueText.setPosition(winW / 2.f, winH / 2.f + 50.f);
            continueText.setFillColor(sf::Color(180, 180, 180));
            window_.draw(continueText);
            return;
        }
        
        // Relic grid layout
        float relicWidth = 200.f;
        float relicHeight = 200.f;
        float padding = 20.f;
        float startX = padding;
        float startY = 120.f; // Below title
        int relicsPerRow = std::max(1, static_cast<int>((winW - padding) / (relicWidth + padding)));
        
        for (size_t i = 0; i < relicsToDisplay_.size(); ++i) {
            const Relic* relic = relicsToDisplay_[i];
            if (!relic) continue;
            
            int row = i / relicsPerRow;
            int col = i % relicsPerRow;
            float x = startX + col * (relicWidth + padding);
            float y = startY + row * (relicHeight + padding);
            
            // Relic background
            sf::RectangleShape relicBg(sf::Vector2f(relicWidth, relicHeight));
            relicBg.setPosition(x, y);
            
            // Set relic background color based on rarity
            sf::Color bgColor;
            switch (relic->getRarity()) {
                case RelicRarity::COMMON: bgColor = sf::Color(120, 120, 120, 220); break;  // Gray for common
                case RelicRarity::UNCOMMON: bgColor = sf::Color(60, 160, 100, 220); break; // Green for uncommon
                case RelicRarity::RARE: bgColor = sf::Color(60, 100, 180, 220); break;     // Blue for rare
                case RelicRarity::BOSS: bgColor = sf::Color(180, 60, 60, 220); break;      // Red for boss
                case RelicRarity::EVENT: bgColor = sf::Color(160, 100, 60, 220); break;    // Orange for event
                case RelicRarity::SHOP: bgColor = sf::Color(160, 130, 60, 220); break;     // Gold for shop
                default: bgColor = sf::Color(120, 120, 120, 220); break;                   // Gray default
            }
            
            relicBg.setFillColor(bgColor);
            relicBg.setOutlineColor(i == selectedIndex_ ? sf::Color::Yellow : sf::Color(200, 200, 200));
            relicBg.setOutlineThickness(i == selectedIndex_ ? 3.f : 1.f);
            window_.draw(relicBg);
            
            // Display index
            sf::Text idxText;
            idxText.setFont(font_);
            idxText.setCharacterSize(20);
            idxText.setFillColor(sf::Color::White);
            idxText.setOutlineColor(sf::Color::Black);
            idxText.setOutlineThickness(1.f);
            idxText.setString(std::to_string(i + 1));
            idxText.setPosition(x + 10.f, y + 10.f);
            window_.draw(idxText);
            
            // Relic name
            sf::Text nameText;
            nameText.setFont(font_);
            nameText.setCharacterSize(18);
            nameText.setFillColor(sf::Color::White);
            nameText.setString(relic->getName());
            sf::FloatRect nameBounds = nameText.getLocalBounds();
            nameText.setOrigin(nameBounds.left + nameBounds.width / 2.f, nameBounds.top);
            nameText.setPosition(x + relicWidth / 2.f, y + 20.f);
            window_.draw(nameText);
            
            // Relic rarity
            std::string rarityStr;
            switch (relic->getRarity()) {
                case RelicRarity::COMMON: rarityStr = "Common"; break;
                case RelicRarity::UNCOMMON: rarityStr = "Uncommon"; break;
                case RelicRarity::RARE: rarityStr = "Rare"; break;
                case RelicRarity::BOSS: rarityStr = "Boss"; break;
                case RelicRarity::STARTER: rarityStr = "Starter"; break;
                case RelicRarity::EVENT: rarityStr = "Event"; break;
                case RelicRarity::SHOP: rarityStr = "Shop"; break;
                default: rarityStr = "Unknown"; break;
            }
            
            sf::Text rarityText;
            rarityText.setFont(font_);
            rarityText.setCharacterSize(14);
            rarityText.setFillColor(sf::Color(230, 230, 230));
            rarityText.setString(rarityStr);
            sf::FloatRect rarityBounds = rarityText.getLocalBounds();
            rarityText.setOrigin(rarityBounds.left + rarityBounds.width / 2.f, rarityBounds.top);
            rarityText.setPosition(x + relicWidth / 2.f, y + 45.f);
            window_.draw(rarityText);
            
            // Relic description - word wrap
            std::string desc = relic->getDescription();
            std::vector<std::string> descLines;
            std::string currentLine;
            std::istringstream descStream(desc);
            std::string word;
            
            while (descStream >> word) {
                sf::Text testText;
                testText.setFont(font_);
                testText.setCharacterSize(14);
                testText.setString(currentLine + " " + word);
                
                if (testText.getLocalBounds().width > (relicWidth - 20.f)) {
                    if (!currentLine.empty()) {
                        descLines.push_back(currentLine);
                        currentLine = word;
                    } else {
                        descLines.push_back(word);
                        currentLine = "";
                    }
                } else {
                    if (currentLine.empty()) {
                        currentLine = word;
                    } else {
                        currentLine += " " + word;
                    }
                }
            }
            
            if (!currentLine.empty()) {
                descLines.push_back(currentLine);
            }
            
            // Draw each line of description
            float descY = y + 70.f;
            for (size_t j = 0; j < descLines.size() && j < 5; ++j) { // Limit to 5 lines
                sf::Text lineText;
                lineText.setFont(font_);
                lineText.setCharacterSize(14);
                lineText.setFillColor(sf::Color::White);
                lineText.setString(descLines[j]);
                sf::FloatRect lineBounds = lineText.getLocalBounds();
                lineText.setOrigin(lineBounds.left + lineBounds.width / 2.f, lineBounds.top);
                lineText.setPosition(x + relicWidth / 2.f, descY);
                window_.draw(lineText);
                descY += lineText.getLocalBounds().height + 5.f;
            }
            
            // If there are more lines than can fit, show ellipsis
            if (descLines.size() > 5) {
                sf::Text ellipsisText("...", font_, 14);
                sf::FloatRect ellipsisBounds = ellipsisText.getLocalBounds();
                ellipsisText.setOrigin(ellipsisBounds.left + ellipsisBounds.width / 2.f, ellipsisBounds.top);
                ellipsisText.setPosition(x + relicWidth / 2.f, descY);
                ellipsisText.setFillColor(sf::Color::White);
                window_.draw(ellipsisText);
            }
        }
        
        // Draw "Close" option at the bottom
        if (!options_.empty()) {
            sf::Text closeText("Close", font_, 24);
            sf::FloatRect closeBounds = closeText.getLocalBounds();
            closeText.setOrigin(closeBounds.left + closeBounds.width / 2.f, closeBounds.top + closeBounds.height / 2.f);
            closeText.setPosition(winW / 2.f, startY + ((relicsToDisplay_.size() + relicsPerRow - 1) / relicsPerRow) * (relicHeight + padding) + 40.f);
            closeText.setFillColor(selectedIndex_ == options_.size() - 1 ? sf::Color::Yellow : sf::Color::White);
            window_.draw(closeText);
        }
    } else {
        for (size_t i = 0; i < options_.size(); ++i) {
            std::string label = options_[i];
            if (!optionInputs_.empty() && i < optionInputs_.size()){
                 if (screenType_ == ScreenType::MainMenu || screenType_ == ScreenType::CharacterSelect || screenType_ == ScreenType::Event) {
                     label = std::to_string(i + 1) + ". " + options_[i];
                 }
            }

            sf::Text opt(label, font_, 32);
            sf::FloatRect optBounds = opt.getLocalBounds();
            opt.setOrigin(optBounds.left + optBounds.width / 2.0f,
                          optBounds.top + optBounds.height / 2.0f);
            opt.setPosition(winW / 2.0f, 100.f + i * 50.f);
            opt.setFillColor(i == selectedIndex_ ? sf::Color::Yellow : sf::Color::White);
            window_.draw(opt);
        }
    }
}

} // namespace deckstiny