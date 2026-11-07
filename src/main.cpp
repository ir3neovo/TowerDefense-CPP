#include "Game.hpp"
#include "PixelText.hpp"
#include <algorithm>
#include <cmath>
#include <string>

namespace {
constexpr float HUD_HEIGHT = 160.f;
const sf::FloatRect startButton({620.f, 20.f}, {160.f, 40.f});
const sf::FloatRect restartButton({620.f, 70.f}, {160.f, 36.f});

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
    sf::RenderWindow window(sf::VideoMode({800, 760}), "Tower Defense - First Stand",
                            sf::Style::Titlebar | sf::Style::Close);
    window.setFramerateLimit(60);
    window.setKeyRepeatEnabled(false);
    const sf::View boardView(sf::FloatRect({0.f, -HUD_HEIGHT}, {800.f, 760.f}));
    Game game;
    sf::Clock clock;
    float accumulator = 0.f;
    constexpr float STEP = 1.f / 60.f;
    std::string notice = "PLACE TOWERS NEAR THE PATH. EACH COSTS 50.";
    float noticeTime = 5.f;

    auto primaryAction = [&] {
        if (game.phase() == Phase::Planning) game.startWave();
        else if (game.phase() == Phase::Running) game.togglePause();
        accumulator = 0.f;
    };
    auto restart = [&] {
        game.reset();
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
            }
            if (const auto* click = event->getIf<sf::Event::MouseButtonPressed>()) {
                if (click->button != sf::Mouse::Button::Left) continue;
                const auto screen = window.mapPixelToCoords(click->position, window.getDefaultView());
                if (startButton.contains(screen)) { primaryAction(); continue; }
                if (restartButton.contains(screen)) { restart(); continue; }
                const auto board = window.mapPixelToCoords(click->position, boardView);
                const auto cell = game.map.pixelToCell({static_cast<int>(std::floor(board.x)), static_cast<int>(std::floor(board.y))});
                if (cell && !game.placeTower(*cell)) {
                    notice = game.paused() ? "RESUME TO BUILD." : "CANNOT BUILD: CHECK GRASS, SPACE AND COINS.";
                    noticeTime = 2.5f;
                }
            }
        }
        if (!window.isOpen()) break;
        if (game.phase() == Phase::Running && !game.paused()) {
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

        if (hovered && (game.phase() == Phase::Planning || game.phase() == Phase::Running)) {
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
        }
        for (const auto& enemy : game.enemies()) {
            const auto position = game.map.positionAlongRoute(enemy.distance);
            sf::CircleShape body(11.f, 8);
            body.setOrigin({11.f, 11.f});
            body.setPosition(position);
            body.setFillColor(sf::Color(205, 94, 75));
            window.draw(body);
            rectangle(window, position + sf::Vector2f{-12.f, -18.f}, {24.f, 4.f}, sf::Color(50, 39, 35));
            rectangle(window, position + sf::Vector2f{-12.f, -18.f}, {24.f * enemy.health / 60.f, 4.f}, sf::Color(235, 199, 106));
        }
        for (const auto& shot : game.shots()) {
            sf::Vertex beam[2];
            beam[0].position = shot.from;
            beam[1].position = shot.to;
            beam[0].color = beam[1].color = sf::Color(247, 230, 157);
            window.draw(beam, 2, sf::PrimitiveType::Lines);
        }
        PixelText::draw(window, "IN", {3.f, 166.f}, 1.f, sf::Color(38, 50, 40));
        PixelText::draw(window, "OUT", {779.f, 246.f}, 1.f, sf::Color(38, 50, 40));

        window.setView(window.getDefaultView());
        rectangle(window, {0.f, 0.f}, {800.f, HUD_HEIGHT}, sf::Color(25, 38, 34));
        PixelText::draw(window, "FIRST STAND", {20.f, 18.f}, 3.f);
        PixelText::draw(window, "COINS " + std::to_string(game.coins()) + "   LIVES " + std::to_string(game.lives()) + "/5", {20.f, 54.f});
        PixelText::draw(window, "WAVE 1/1   SPAWNED " + std::to_string(game.spawned()) + "/12   KILLS " + std::to_string(game.defeated()), {20.f, 80.f});
        PixelText::draw(window, "CLICK GRASS: TOWER 50   SPACE: START/PAUSE   R: RESTART", {20.f, 112.f}, 1.5f);
        PixelText::draw(window, noticeTime > 0.f ? notice : "STOP THE WAVE. EACH ESCAPE COSTS A LIFE. KILLS EARN 15.", {20.f, 136.f}, 1.5f, sf::Color(175, 193, 166));
        const std::string label = game.phase() == Phase::Planning ? "START WAVE"
            : game.phase() == Phase::Running ? (game.paused() ? "RESUME" : "PAUSE") : "COMPLETE";
        button(window, startButton, label, screenMouse, sf::Color(60, 98, 73));
        button(window, restartButton, "RESTART", screenMouse, sf::Color(61, 72, 62));
        if (game.paused() || game.phase() == Phase::Won || game.phase() == Phase::Lost) {
            rectangle(window, {0.f, HUD_HEIGHT}, {800.f, 600.f}, sf::Color(15, 26, 22, 165));
            rectangle(window, {130.f, 350.f}, {540.f, 170.f}, sf::Color(29, 46, 38));
            const std::string title = game.paused() ? "PAUSED" : game.phase() == Phase::Won ? "WAVE CLEARED!" : "DEFENSE FALLEN";
            PixelText::draw(window, title, {170.f, 380.f}, 3.f);
            PixelText::draw(window, "DEFEATED " + std::to_string(game.defeated()) + "   ESCAPED " + std::to_string(game.escaped()), {170.f, 430.f});
            PixelText::draw(window, game.paused() ? "SPACE TO RESUME" : "PRESS R TO TRY A NEW DEFENSE", {170.f, 470.f}, 2.f);
        }
        window.display();
    }
}
