#include "Game.hpp"
#include "EnemyBehavior.hpp"
#include "PixelText.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr float HUD_HEIGHT = 220.f;
const sf::FloatRect startButton({620.f, 20.f}, {160.f, 40.f});
const sf::FloatRect restartButton({620.f, 70.f}, {160.f, 36.f});
const sf::FloatRect helpButton({620.f, 116.f}, {160.f, 36.f});

void rectangle(sf::RenderWindow& window, sf::Vector2f at, sf::Vector2f size, sf::Color color) {
    sf::RectangleShape shape(size);
    shape.setPosition(at);
    shape.setFillColor(color);
    window.draw(shape);
}

void button(sf::RenderWindow& window, const sf::FloatRect& bounds, const std::string& label,
            sf::Vector2f mouse, sf::Color color) {
    rectangle(window, bounds.position, bounds.size, bounds.contains(mouse) ? sf::Color(99, 135, 105) : color);
    PixelText::draw(window, label, bounds.position + sf::Vector2f{12.f, 12.f});
}
}

int main() {
    sf::RenderWindow window(sf::VideoMode({800, 820}), "Tower Defense - First Stand",
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
    const sf::View boardView(sf::FloatRect({0.f, -HUD_HEIGHT}, {800.f, 820.f}));
    Game game;
    bool helpVisible = false;
    sf::Clock clock;
    float accumulator = 0.f;
    constexpr float STEP = 1.f / 60.f;
    std::string notice = "PLACE TOWERS NEAR THE PATH. EACH COSTS 50.";
    float noticeTime = 5.f;

    auto primaryAction = [&] {
        helpVisible = false;
        if (game.phase() == Phase::Planning) game.startWave();
        else if (game.phase() == Phase::Running) game.togglePause();
        accumulator = 0.f;
    };
    auto restart = [&] {
        game.reset();
        helpVisible = false;
        accumulator = 0.f;
        notice = "NEW DEFENSE. PLACE TOWERS THEN START THE WAVE.";
        noticeTime = 5.f;
    };
    while (window.isOpen()) {
        const float elapsed = std::min(clock.restart().asSeconds(), 0.25f);
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (event->is<sf::Event::FocusLost>() && game.phase() == Phase::Running && !game.paused())
                game.togglePause();
            if (const auto* key = event->getIf<sf::Event::KeyPressed>()) {
                if (key->code == sf::Keyboard::Key::Escape) window.close();
                if (key->code == sf::Keyboard::Key::Space) primaryAction();
                if (key->code == sf::Keyboard::Key::P) game.togglePause();
                if (key->code == sf::Keyboard::Key::R) restart();
                if (key->code == sf::Keyboard::Key::H) { helpVisible = !helpVisible; accumulator = 0.f; }
            }
            if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>()) {
                const auto screen = window.mapPixelToCoords(click->position, window.getDefaultView());
                if (click->button == sf::Mouse::Button::Left) {
                    if (startButton.contains(screen)) { primaryAction(); continue; }
                    if (restartButton.contains(screen)) { restart(); continue; }
                    if (helpButton.contains(screen)) { helpVisible = !helpVisible; accumulator = 0.f; continue; }
                }
                if (helpVisible) continue;
                const auto board = window.mapPixelToCoords(click->position, boardView);
                const auto cell = game.map.pixelToCell({static_cast<int>(std::floor(board.x)), static_cast<int>(std::floor(board.y))});
                if (cell && click->button == sf::Mouse::Button::Right && game.toggleTargeting(*cell)) {
                    for (const auto& tower : game.towers()) if (tower.cell == *cell)
                        notice = tower.targeting == Targeting::HealersFirst ? "TARGET: HEALERS FIRST. H MARKS THIS TOWER." : "TARGET: FIRST ENEMY ALONG THE ROUTE.";
                    noticeTime = 3.f;
                }
                if (cell && click->button == sf::Mouse::Button::Left && !game.placeTower(*cell)) {
                    notice = game.paused() ? "RESUME TO BUILD." : "CANNOT BUILD: CHECK GRASS, SPACE AND COINS.";
                    noticeTime = 2.5f;
                }
            }
        }
        if (!window.isOpen()) break;
        if (game.phase() == Phase::Running && !game.paused() && !helpVisible) {
            accumulator += elapsed;
            while (accumulator >= STEP) { game.update(STEP); accumulator -= STEP; }
        } else accumulator = 0.f;
        noticeTime = std::max(0.f, noticeTime - elapsed);

        const auto mousePixel = sf::Mouse::getPosition(window);
        const auto screenMouse = window.mapPixelToCoords(mousePixel, window.getDefaultView());
        const auto boardMouse = window.mapPixelToCoords(mousePixel, boardView);
        const auto hovered = game.map.pixelToCell({static_cast<int>(std::floor(boardMouse.x)), static_cast<int>(std::floor(boardMouse.y))});
        window.clear(sf::Color(30, 40, 36));
        window.setView(boardView);
        game.map.draw(window, hovered);

        if (!helpVisible && hovered && (game.phase() == Phase::Planning || game.phase() == Phase::Running)) {
            const auto center = GameMap::cellCenter(*hovered);
            const bool valid = game.canPlace(*hovered);
            sf::CircleShape range(Game::RANGE, 64);
            range.setOrigin({Game::RANGE, Game::RANGE});
            range.setPosition(center);
            range.setFillColor(valid ? sf::Color(148, 205, 179, 25) : sf::Color(230, 100, 100, 18));
            range.setOutlineThickness(1.f);
            range.setOutlineColor(valid ? sf::Color(173, 222, 190, 150) : sf::Color(220, 115, 115, 120));
            window.draw(range);
            rectangle(window, center - sf::Vector2f{10.f, 10.f}, {20.f, 20.f},
                      valid ? sf::Color(160, 220, 185, 150) : sf::Color(220, 100, 100, 150));
        }
        for (const auto& tower : game.towers()) {
            const auto center = GameMap::cellCenter(tower.cell);
            rectangle(window, center - sf::Vector2f{14.f, 14.f}, {28.f, 28.f}, sf::Color(31, 55, 54));
            sf::CircleShape turret(10.f, 8);
            turret.setOrigin({10.f, 10.f});
            turret.setPosition(center);
            turret.setFillColor(sf::Color(126, 200, 178));
            window.draw(turret);
            rectangle(window, center + sf::Vector2f{-3.f, -15.f}, {6.f, 15.f}, sf::Color(211, 232, 208));
            if (tower.targeting == Targeting::HealersFirst) PixelText::draw(window, "H", center + sf::Vector2f{-2.f, -3.f}, 1.f, sf::Color(20, 50, 40));
        }
        for (const auto& enemy : game.enemies()) {
            const auto position = game.map.positionAlongRoute(enemy.distance);
            const auto facing = game.map.directionAlongRoute(enemy.distance);
            sf::Color color(205, 94, 75);
            if (enemy.kind == EnemyKind::Shield) color = sf::Color(114, 173, 225);
            if (enemy.kind == EnemyKind::Healer) color = sf::Color(120, 225, 155);
            if (enemy.kind == EnemyKind::Scout) color = sf::Color(241, 190, 80);
            if (enemy.kind == EnemyKind::Scout && enemy.weakPoint >= 0.f
                && EnemyBehavior::coverageAt(enemy.weakPoint, game.towers(), game.map, Game::RANGE) < 2) {
                const auto gap = game.map.positionAlongRoute(enemy.weakPoint);
                sf::CircleShape marker(18.f, 4);
                marker.setOrigin({18.f, 18.f}); marker.setPosition(gap);
                marker.setFillColor(sf::Color::Transparent); marker.setOutlineThickness(2.f);
                marker.setOutlineColor(sf::Color(241, 190, 80, 170)); window.draw(marker);
                if (enemy.state == EnemyState::Warning) PixelText::draw(window, "!", position + sf::Vector2f{-2.f, -31.f}, 2.f, color);
            }
            if (enemy.kind == EnemyKind::Healer && enemy.state == EnemyState::Healing) {
                sf::CircleShape aura(EnemyBehavior::HEAL_RANGE, 40);
                aura.setOrigin({EnemyBehavior::HEAL_RANGE, EnemyBehavior::HEAL_RANGE}); aura.setPosition(position);
                aura.setFillColor(sf::Color(120, 225, 155, 20)); aura.setOutlineThickness(1.f);
                aura.setOutlineColor(sf::Color(120, 225, 155, 100)); window.draw(aura);
            }
            if (enemy.state == EnemyState::Dashing) {
                for (int i = 1; i <= 3; ++i) {
                    const auto tail = game.map.positionAlongRoute(enemy.distance - i * 9.f);
                    rectangle(window, tail - sf::Vector2f{3.f, 3.f}, {6.f, 6.f}, sf::Color(241, 190, 80, 160 - i * 35));
                }
            }
            sf::CircleShape body(11.f, enemy.kind == EnemyKind::Scout ? 3 : 8);
            body.setOrigin({11.f, 11.f}); body.setPosition(position); body.setFillColor(color);
            window.draw(body);
            if (enemy.kind == EnemyKind::Shield) {
                const auto guard = position + facing * 14.f;
                const bool horizontal = facing.x != 0.f;
                const sf::Vector2f size = horizontal ? sf::Vector2f{4.f, 22.f} : sf::Vector2f{22.f, 4.f};
                rectangle(window, guard - size / 2.f, size,
                    enemy.state == EnemyState::Guarding ? sf::Color::White : sf::Color(50, 108, 179));
            }
            if (enemy.kind == EnemyKind::Healer) PixelText::draw(window, "+", position - sf::Vector2f{5.f, 7.f}, 2.f, sf::Color(20, 75, 45));
            rectangle(window, position + sf::Vector2f{-12.f, -18.f}, {24.f, 4.f}, sf::Color(50, 39, 35));
            rectangle(window, position + sf::Vector2f{-12.f, -18.f}, {24.f * enemy.health / enemy.maxHealth, 4.f}, sf::Color(235, 199, 106));
        }
        for (const auto& shot : game.shots()) {
            sf::Vertex beam[2];
            beam[0].position = shot.from;
            beam[1].position = shot.to;
            beam[0].color = beam[1].color = shot.kind == ShotKind::Heal ? sf::Color(120, 255, 170)
                : shot.kind == ShotKind::Blocked ? sf::Color(135, 200, 255) : sf::Color(247, 230, 157);
            window.draw(beam, 2, sf::PrimitiveType::Lines);
        }
        PixelText::draw(window, "IN", {3.f, 166.f}, 1.f, sf::Color(38, 50, 40));
        PixelText::draw(window, "OUT", {779.f, 246.f}, 1.f, sf::Color(38, 50, 40));

        window.setView(window.getDefaultView());
        rectangle(window, {0.f, 0.f}, {800.f, HUD_HEIGHT}, sf::Color(25, 38, 34));
        PixelText::draw(window, "FIRST STAND", {20.f, 18.f}, 3.f);
        PixelText::draw(window, "COINS " + std::to_string(game.coins()) + "   LIVES " + std::to_string(game.lives()) + "/5", {20.f, 54.f});
        PixelText::draw(window, "WAVE 1/1   SPAWNED " + std::to_string(game.spawned()) + "/12   KILLS " + std::to_string(game.defeated()), {20.f, 80.f});
        PixelText::draw(window, "CLICK: BUILD 50   RIGHT CLICK TOWER: TARGET", {20.f, 112.f}, 2.f);
        PixelText::draw(window, "BLUE: FLANK SHIELDS   GREEN: STOP HEALERS", {20.f, 144.f}, 2.f);
        PixelText::draw(window, "GOLD SCOUTS: OVERLAP RANGES TO STOP SPRINTS", {20.f, 170.f}, 2.f, sf::Color(241, 190, 110));
        PixelText::draw(window, noticeTime > 0.f ? notice : "SPACE: START/PAUSE   R: RESTART   H: HELP", {20.f, 198.f}, 2.f, sf::Color(175, 193, 166));
        const std::string label = game.phase() == Phase::Planning ? "START WAVE"
            : game.phase() == Phase::Running ? (game.paused() ? "RESUME" : "PAUSE") : "COMPLETE";
        button(window, startButton, label, screenMouse, sf::Color(60, 98, 73));
        button(window, restartButton, "RESTART", screenMouse, sf::Color(61, 72, 62));
        button(window, helpButton, helpVisible ? "CLOSE HELP" : "HELP - H", screenMouse, sf::Color(61, 72, 62));
        if (game.paused() || game.phase() == Phase::Won || game.phase() == Phase::Lost) {
            rectangle(window, {0.f, HUD_HEIGHT}, {800.f, 600.f}, sf::Color(15, 26, 22, 165));
            rectangle(window, {100.f, 350.f}, {600.f, 210.f}, sf::Color(29, 46, 38));
            const std::string title = game.paused() ? "PAUSED" : game.phase() == Phase::Won ? "WAVE CLEARED!" : "DEFENSE FALLEN";
            PixelText::draw(window, title, {170.f, 380.f}, 3.f);
            PixelText::draw(window, "DEFEATED " + std::to_string(game.defeated()) + "   ESCAPED " + std::to_string(game.escaped()), {170.f, 430.f});
            PixelText::draw(window, "BLOCKED " + std::to_string(game.blockedDamage()) + "  HEALED " + std::to_string(game.healedHealth()) + "  DASHES " + std::to_string(game.scoutDashes()), {130.f, 470.f}, 1.5f);
            PixelText::draw(window, game.paused() ? "SPACE TO RESUME" : "PRESS R TO TRY A NEW DEFENSE", {170.f, 510.f}, 2.f);
        }
        if (helpVisible) {
            rectangle(window, {0.f, HUD_HEIGHT}, {800.f, 600.f}, sf::Color(15, 26, 22, 215));
            PixelText::draw(window, "KNOW YOUR ENEMIES", {52.f, 270.f}, 3.f);
            PixelText::draw(window, "BLUE SHIELDS BLOCK 75 PERCENT FROM THE FRONT.\nPLACE TOWERS TO THEIR SIDES OR BEHIND THEM.", {52.f, 330.f}, 2.f);
            PixelText::draw(window, "GREEN HEALERS RESTORE WOUNDED NEARBY ALLIES.\nRIGHT CLICK A TOWER TO PRIORITIZE HEALERS.\nH ON A TOWER MEANS HEALERS FIRST.", {52.f, 400.f}, 2.f);
            PixelText::draw(window, "GOLD SCOUTS MARK WEAK SPOTS WITH DIAMONDS.\nAFTER A WARNING THEY SPRINT THROUGH GAPS.\nTWO OVERLAPPING TOWERS STOP THEIR SPRINT.", {52.f, 490.f}, 2.f);
            PixelText::draw(window, "CLICK GRASS TO BUY TOWERS. START WITH THREE.\nSPACE: START/PAUSE   R: RESTART\nH OR CLOSE HELP: RETURN TO THE BOARD", {52.f, 590.f}, 2.f);
        }
        window.display();
    }
}
