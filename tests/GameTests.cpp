#include "Game.hpp"
#include "TestRunner.hpp"
#include <cmath>
#include <stdexcept>

void check(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void runWave(Game& game) {
    game.startWave();
    for (int step = 0; step < 60 * 90 && game.phase() == Phase::Running; ++step) game.update(1.f / 60.f);
    check(game.phase() != Phase::Running, "Wave must terminate");
}
void buildStarter(Game& game) {
    check(game.placeTower({3, 3}) && game.placeTower({9, 9}) && game.placeTower({16, 5}), "Starter layout must be placeable");
}
void routeContinuity() {
    GameMap map;
    const auto& route = map.route();
    check(route.front() == sf::Vector2i{-1, 4}, "Entry must be off board");
    check(route.back() == sf::Vector2i{20, 6}, "Exit must be off board");
    for (std::size_t i = 1; i < route.size(); ++i) {
        check(std::abs(route[i].x - route[i-1].x) + std::abs(route[i].y - route[i-1].y) == 1,
              "Route cells must be adjacent without duplicate corners");
        check(!map.isBuildable(route[i]), "Route must never be buildable");
    }
}
void routeInterpolation() {
    GameMap map;
    const auto& route = map.route();
    for (std::size_t i = 0; i < route.size(); ++i)
        check(map.positionAlongRoute(static_cast<float>(i) * GameMap::TILE_SIZE) == GameMap::cellCenter(route[i]),
              "Movement must visit every turn in order");
    check(map.positionAlongRoute(-50.f) == GameMap::cellCenter(route.front()), "Negative travel clamps to entry");
    check(map.positionAlongRoute(map.routeLength() + 50.f) == GameMap::cellCenter(route.back()), "Excess travel clamps to exit");
    check(map.positionAlongRoute(20.f) == sf::Vector2f{0.f, 180.f}, "Half-segment movement must interpolate");
}
void mapBoundaries() {
    GameMap map;
    check(!map.pixelToCell({-1, 0}) && !map.pixelToCell({0, -1}), "Negative pixels are outside");
    check(!map.pixelToCell({800, 599}) && !map.pixelToCell({0, 600}), "Right and bottom edges are outside");
    check(map.pixelToCell({0, 0}) == sf::Vector2i{0, 0}, "Top left pixel is inside");
    check(map.pixelToCell({799, 599}) == sf::Vector2i{19, 14}, "Last pixel maps to last cell");
}
void placementRules() {
    Game game;
    check(!game.placeTower({0, 4}), "Cannot build on route");
    check(!game.placeTower({-1, 0}) && !game.placeTower({20, 15}), "Cannot build outside board");
    check(game.coins() == Game::STARTING_COINS, "Invalid purchases must not spend coins");
    check(game.placeTower({3, 3}), "Valid purchase succeeds");
    check(!game.placeTower({3, 3}), "Cannot overlap towers");
    check(game.towers().size() == 1 && game.coins() == 100, "Rejected overlap changes neither towers nor coins");
}
void economy() {
    Game game;
    buildStarter(game);
    check(game.coins() == 0 && !game.placeTower({0, 0}), "Cannot overspend");
    runWave(game);
    check(game.coins() == game.defeated() * Game::KILL_REWARD, "Kills award coins exactly once");
    check(game.lives() == Game::STARTING_LIVES - game.escaped(), "Every escape costs one life");
}
void pauseResume() {
    Game game;
    game.placeTower({3, 3});
    game.startWave();
    game.update(1.f / 60.f);
    const auto before = game.enemies().front().distance;
    const int coins = game.coins();
    game.togglePause();
    game.update(1.f);
    check(game.enemies().front().distance == before, "Pause stops movement");
    check(!game.placeTower({9, 9}) && game.coins() == coins, "Cannot buy while paused even with sufficient funds");
    game.togglePause();
    game.update(1.f / 60.f);
    check(game.enemies().front().distance > before, "Unpause resumes movement");
}
void waveVictory() {
    Game game;
    buildStarter(game);
    runWave(game);
    check(game.phase() == Phase::Won && game.lives() > 0, "Three spaced towers should win");
    check(game.spawned() == Game::WAVE_SIZE && game.enemies().empty(), "Victory requires full wave completion");
    check(game.defeated() + game.escaped() == Game::WAVE_SIZE, "Every enemy resolves once");
}
void waveDefeat() {
    Game game;
    runWave(game);
    check(game.phase() == Phase::Lost && game.lives() == 0, "No towers must lose");
    check(game.defeated() == 0 && game.escaped() == Game::STARTING_LIVES, "Defeat occurs at the fifth escape");
}
void terminalStates() {
    for (const bool defended : {false, true}) {
        Game game;
        if (defended) buildStarter(game);
        runWave(game);
        const auto phase = game.phase();
        const int coins = game.coins(), lives = game.lives(), spawned = game.spawned();
        const auto count = game.enemies().size();
        const float distance = count ? game.enemies().front().distance : 0.f;
        check(!game.placeTower({0, 0}), "Cannot build after a result");
        check(!game.toggleTargeting({3, 3}), "Cannot retarget after a result");
        game.startWave(); game.togglePause(); game.update(10.f);
        check(game.phase() == phase && game.coins() == coins && game.lives() == lives
              && game.spawned() == spawned && !game.paused(), "Terminal state must stay frozen");
        check(game.enemies().size() == count && (!count || game.enemies().front().distance == distance), "Remaining enemies freeze after defeat");
    }
}
void restart() {
    Game game;
    game.placeTower({7, 4}); game.toggleTargeting({7, 4}); game.startWave();
    for (int i = 0; i < 600; ++i) game.update(1.f / 60.f);
    game.togglePause(); game.reset();
    check(game.phase() == Phase::Planning && game.towers().empty() && game.enemies().empty()
          && game.shots().empty() && game.coins() == 150 && game.lives() == 5 && !game.paused(), "Reset restores initial state");
    check(game.spawned() == 0 && game.defeated() == 0 && game.escaped() == 0
          && game.blockedDamage() == 0 && game.healedHealth() == 0 && game.scoutDashes() == 0, "Reset clears every counter");
    game.startWave(); game.update(1.f / 60.f);
    check(game.spawned() == 1 && game.enemies().front().kind == EnemyKind::Grunt, "Restart resets wave schedule");
}
int main(int argc, char** argv) {
    return runTests(argc, argv, {
        {"route_continuity", routeContinuity}, {"route_interpolation", routeInterpolation},
        {"map_boundaries", mapBoundaries}, {"placement_rules", placementRules}, {"economy", economy},
        {"pause_resume", pauseResume}, {"wave_victory", waveVictory}, {"wave_defeat", waveDefeat},
        {"terminal_states", terminalStates}, {"restart", restart}
    });
}
