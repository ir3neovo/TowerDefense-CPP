#include "Game.hpp"
#include <cmath>
#include <iostream>
#include <stdexcept>

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void runWave(Game& game) {
    game.startWave();
    for (int step = 0; step < 60 * 90 && game.phase() == Phase::Running; ++step) game.update(1.f / 60.f);
    check(game.phase() != Phase::Running, "Wave must terminate");
}
int main() {
    try {
        Game game;
        const auto& route = game.map.route();
        check(route.front() == sf::Vector2i{-1, 4}, "Entry must be off board");
        check(route.back() == sf::Vector2i{20, 6}, "Exit must be off board");
        for (std::size_t i = 1; i < route.size(); ++i) {
            check(std::abs(route[i].x - route[i-1].x) + std::abs(route[i].y - route[i-1].y) == 1,
                  "Route cells must be adjacent without duplicate corners");
            check(!game.map.isBuildable(route[i]), "Route must never be buildable");
            const auto position = game.map.positionAlongRoute(static_cast<float>(i) * GameMap::TILE_SIZE);
            check(position == GameMap::cellCenter(route[i]), "Movement must visit every turn in order");
        }
        check(game.map.positionAlongRoute(-50.f) == GameMap::cellCenter(route.front()), "Negative travel must clamp to entry");
        check(game.map.positionAlongRoute(game.map.routeLength() + 50.f) == GameMap::cellCenter(route.back()), "Excess travel must clamp to exit");
        check(game.map.positionAlongRoute(20.f) == sf::Vector2f{0.f, 180.f}, "Half-segment movement must interpolate");
        check(!game.map.pixelToCell({-1, 0}), "Outside pixels must not map onto grass");
        check(!game.map.pixelToCell({800, 599}), "Right edge is outside the board");
        check(!game.map.pixelToCell({0, 600}), "Bottom edge is outside the board");
        check(!game.placeTower({0, 4}), "Cannot build on route");
        check(!game.placeTower({-1, 0}), "Cannot build out of bounds");
        check(game.coins() == Game::STARTING_COINS, "Invalid purchases must not spend coins");
        check(game.placeTower({3, 3}), "Valid purchase should succeed");
        check(!game.placeTower({3, 3}), "Cannot overlap towers");
        check(game.placeTower({9, 9}) && game.placeTower({16, 5}), "Three starter towers should be affordable");
        check(game.coins() == 0 && !game.placeTower({0, 0}), "Cannot overspend");
        game.startWave();
        game.update(1.f / 60.f);
        const float before = game.enemies().front().distance;
        game.togglePause();
        game.update(1.f);
        check(game.enemies().front().distance == before, "Pause must stop movement");
        check(!game.placeTower({0, 0}), "Cannot place while paused");
        game.togglePause();
        runWave(game);
        check(game.phase() == Phase::Won, "Three spaced towers should beat the starter wave");
        check(game.defeated() + game.escaped() == Game::WAVE_SIZE, "Every enemy must resolve once");
        check(game.coins() == game.defeated() * Game::KILL_REWARD, "Kills must award coins exactly once");
        check(game.lives() == Game::STARTING_LIVES - game.escaped(), "Each escape costs one life");
        check(!game.placeTower({0, 0}), "Terminal state must reject placement");
        std::cout << "Winning layout: " << game.defeated() << " kills, " << game.escaped() << " escapes\n";
        game.reset();
        check(game.phase() == Phase::Planning && game.towers().empty() && game.enemies().empty()
              && game.shots().empty() && game.coins() == 150 && game.lives() == 5 && !game.paused(), "Reset must restore the full state");
        runWave(game);
        check(game.phase() == Phase::Lost && game.lives() == 0, "No towers must lose");
        const int spawned = game.spawned();
        game.update(10.f);
        check(game.spawned() == spawned && game.lives() == 0, "Terminal states must be frozen");
        std::cout << "Route, placement, economy, pause, victory, defeat and reset checks passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

