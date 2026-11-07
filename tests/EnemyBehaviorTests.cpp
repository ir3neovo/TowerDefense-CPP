#include "Game.hpp"
#include "TestRunner.hpp"
#include "EnemyBehavior.hpp"
#include <array>
#include <iostream>
#include <stdexcept>

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void shields() {
    GameMap map;
    auto shield = EnemyBehavior::makeEnemy(EnemyKind::Shield);
    shield.distance = 100.f;
    auto at = map.positionAlongRoute(shield.distance);
    require(EnemyBehavior::damageFrom(shield, at + sf::Vector2f{80.f, 0.f}, map, 20) == 5, "Front shield must absorb 75 percent");
    require(EnemyBehavior::damageFrom(shield, at + sf::Vector2f{-80.f, 0.f}, map, 20) == 20, "Rear hit must bypass shield");
    require(EnemyBehavior::damageFrom(shield, at + sf::Vector2f{0.f, 80.f}, map, 20) == 20, "Side hit must bypass shield");
    require(EnemyBehavior::damageFrom(shield, at + sf::Vector2f{80.f, 40.f}, map, 20) == 5, "Diagonal front must be guarded");
    shield.distance = 280.f;
    at = map.positionAlongRoute(shield.distance);
    require(map.directionAlongRoute(280.f) == sf::Vector2f{0.f, 1.f}, "Shield heading must turn with route");
    require(EnemyBehavior::damageFrom(shield, at + sf::Vector2f{0.f, 80.f}, map, 20) == 5, "New front must be protected after corner");
    require(EnemyBehavior::damageFrom(shield, at + sf::Vector2f{80.f, 0.f}, map, 20) == 20, "Old front becomes unprotected side");
    shield.health = 0;
    require(EnemyBehavior::damageFrom(shield, at, map, 20) == 0, "Dead enemies must not take another hit");
}

void healers() {
    GameMap map;
    std::vector<Enemy> enemies = {EnemyBehavior::makeEnemy(EnemyKind::Healer),
        EnemyBehavior::makeEnemy(EnemyKind::Shield), EnemyBehavior::makeEnemy(EnemyKind::Grunt)};
    for (auto& enemy : enemies) enemy.distance = 100.f;
    enemies[0].health = 1; enemies[1].health = 40; enemies[2].health = 20;
    require(EnemyBehavior::healingTarget(0, enemies, map) == 2, "Triage must prefer lowest fraction, excluding self");
    std::vector<Shot> effects;
    auto events = EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, .01f, effects);
    require(events.healed == 12 && enemies[2].health == 32, "Heal must restore the selected ally");
    require(enemies[0].state == EnemyState::Healing && EnemyBehavior::movementSpeed(enemies[0]) == 0.f, "Healer must stop while casting");
    require(effects.size() == 1 && effects[0].kind == ShotKind::Heal, "Healing must produce feedback");
    events = EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, .1f, effects);
    require(events.healed == 0, "Heal must not repeat during cooldown");
    enemies[1].health = 80; enemies[2].health = 59;
    events = EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, 1.6f, effects);
    require(events.healed == 1 && enemies[2].health == 60, "Healing must clamp at maximum HP");
    enemies[2].health = 0;
    require(!EnemyBehavior::healingTarget(0, enemies, map), "Dead allies cannot be resurrected");
    enemies[2].health = 10; enemies[2].distance = 500.f;
    require(!EnemyBehavior::healingTarget(0, enemies, map), "Healer must respect radius");
    enemies[2].distance = map.routeLength();
    require(!EnemyBehavior::healingTarget(0, enemies, map), "Escaped allies cannot be healed");
    enemies[0].health = 0; enemies[2].distance = 100.f;
    require(!EnemyBehavior::healingTarget(0, enemies, map), "Dead healers cannot heal");
}

void scouts() {
    GameMap map;
    std::vector<Enemy> enemies{EnemyBehavior::makeEnemy(EnemyKind::Scout)};
    enemies[0].distance = 120.f;
    std::vector<Shot> effects;
    EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, .01f, effects);
    require(enemies[0].state == EnemyState::Warning, "Scout must warn before sprinting");
    require(enemies[0].weakPoint >= enemies[0].distance, "Scout must identify an upcoming weak point");
    auto events = EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, .61f, effects);
    require(events.beganDash && enemies[0].state == EnemyState::Dashing, "Scout must sprint after warning");
    require(EnemyBehavior::movementSpeed(enemies[0]) == EnemyBehavior::SCOUT_DASH_SPEED, "Sprint must increase speed");
    std::vector<Tower> towers{{{2, 3}}, {{3, 3}}};
    require(EnemyBehavior::coverageAt(120.f, towers, map, Game::RANGE) == 2, "Overlapping tower coverage must be counted");
    EnemyBehavior::update(0, enemies, towers, map, Game::RANGE, .01f, effects);
    require(enemies[0].state == EnemyState::Scouting, "Adding overlapping coverage must cancel a sprint");
    EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, .01f, effects);
    require(enemies[0].state == EnemyState::Scouting, "Cooldown must prevent immediate sprint restart");
    EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, 2.5f, effects);
    require(enemies[0].state == EnemyState::Warning, "Scout can warn again after cooldown");
    EnemyBehavior::update(0, enemies, towers, map, Game::RANGE, .1f, effects);
    require(enemies[0].state == EnemyState::Scouting, "Overlapping coverage must cancel the warning too");
    const float gap = EnemyBehavior::weakestPoint(120.f, towers, map, Game::RANGE);
    require(gap >= 120.f && gap <= 280.f, "Gap scan must stay inside lookahead");
    require(EnemyBehavior::coverageAt(gap, towers, map, Game::RANGE) < 2, "Scout should reveal weak coverage ahead");
}

void targetingAndWave() {
    GameMap map;
    Tower tower{{3, 3}};
    std::vector<Enemy> enemies{EnemyBehavior::makeEnemy(EnemyKind::Grunt), EnemyBehavior::makeEnemy(EnemyKind::Healer)};
    enemies[0].distance = 180.f; enemies[1].distance = 120.f;
    require(EnemyBehavior::towerTarget(tower, enemies, map, Game::RANGE) == 0, "Default tower targets furthest enemy");
    tower.targeting = Targeting::HealersFirst;
    require(EnemyBehavior::towerTarget(tower, enemies, map, Game::RANGE) == 1, "Support priority must target healer");
    enemies[1].health = 0;
    require(EnemyBehavior::towerTarget(tower, enemies, map, Game::RANGE) == 0, "Support priority falls back after healer dies");
    enemies[1].health = 45; enemies[1].distance = 900.f;
    require(EnemyBehavior::towerTarget(tower, enemies, map, Game::RANGE) == 0, "Support priority must obey range");
    Game game;
    require(!game.toggleTargeting({0, 0}), "Empty cell has no targeting mode");
    game.placeTower({3, 3}); game.placeTower({9, 9}); game.placeTower({16, 5});
    require(game.toggleTargeting({3, 3}) && game.coins() == 0, "Changing targeting costs no coins");
    game.startWave(); game.update(.01f); game.togglePause();
    const auto snapshot = game.enemies();
    game.update(5.f);
    require(game.enemies()[0].distance == snapshot[0].distance && game.scoutDashes() == 0, "Pause freezes enemy abilities and motion");
    require(!game.toggleTargeting({3, 3}), "Cannot retarget during pause");
    game.togglePause();
    std::array<bool, 4> seen{};
    for (int i = 0; i < 60 * 90 && game.phase() == Phase::Running; ++i) {
        game.update(1.f / 60.f);
        for (const auto& enemy : game.enemies()) seen[static_cast<int>(enemy.kind)] = true;
    }
    for (bool kind : seen) require(kind, "Wave must spawn all four enemy types");
    require(game.phase() == Phase::Won, "Mixed wave must be beatable with three spaced towers");
    std::cout << "Mixed wave: " << game.defeated() << " kills, " << game.escaped() << " escapes; "
              << game.blockedDamage() << " damage blocked, " << game.healedHealth() << " HP healed, " << game.scoutDashes() << " dashes\n";
    Game encounter;
    encounter.placeTower({7, 4}); // Directly ahead of the first straight: deliberately shield-friendly.
    encounter.startWave();
    for (int i = 0; i < 60 * 90 && encounter.phase() == Phase::Running; ++i) encounter.update(1.f / 60.f);
    std::cout << "Frontal layout: blocked=" << encounter.blockedDamage() << " healed=" << encounter.healedHealth() << " dashes=" << encounter.scoutDashes() << '\n';
    require(encounter.blockedDamage() > 0 && encounter.healedHealth() > 0 && encounter.scoutDashes() > 0,
            "Frontal encounter must exercise blocking, healing and scouting");
    game.reset();
    require(game.blockedDamage() == 0 && game.healedHealth() == 0 && game.scoutDashes() == 0, "Restart clears behavior statistics");
}


void scoutCooldownCycle() {
    GameMap map;
    std::vector<Enemy> enemies{EnemyBehavior::makeEnemy(EnemyKind::Scout)};
    enemies[0].distance = 120.f;
    std::vector<Shot> effects;
    constexpr float tick = 1.f / 60.f;
    int dashes = 0;
    float warningTime = 0.f;
    EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, tick, effects);
    while (enemies[0].state == EnemyState::Warning && warningTime < 2.f) {
        auto events = EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, tick, effects);
        if (events.beganDash) ++dashes;
        warningTime += tick;
    }
    require(enemies[0].state == EnemyState::Dashing && dashes == 1, "Warning starts exactly one dash");
    require(warningTime >= EnemyBehavior::SCOUT_WARNING - tick && warningTime <= EnemyBehavior::SCOUT_WARNING + tick * 2,
            "Warning must last its configured duration within timestep tolerance");
    float dashTime = 0.f;
    while (enemies[0].state == EnemyState::Dashing && dashTime < 3.f) {
        const auto events = EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, tick, effects);
        require(!events.beganDash, "A running dash cannot count as another activation");
        dashTime += tick;
    }
    require(enemies[0].state == EnemyState::Scouting, "Dash expires without needing tower coverage");
    require(dashTime >= EnemyBehavior::SCOUT_DASH_TIME - tick && dashTime <= EnemyBehavior::SCOUT_DASH_TIME + tick * 2,
            "Dash duration must match configuration");
    require(EnemyBehavior::movementSpeed(enemies[0]) == 70.f, "Expired dash restores normal speed");
    float cooldownTime = 0.f;
    while (enemies[0].state == EnemyState::Scouting && cooldownTime < 4.f) {
        EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, tick, effects);
        cooldownTime += tick;
    }
    require(enemies[0].state == EnemyState::Warning, "Scout warns again after cooldown");
    require(cooldownTime >= EnemyBehavior::SCOUT_COOLDOWN - tick && cooldownTime <= EnemyBehavior::SCOUT_COOLDOWN + tick * 2,
            "Scout cannot skip its cooldown");
}

void healerCastCycle() {
    GameMap map;
    std::vector<Enemy> enemies{EnemyBehavior::makeEnemy(EnemyKind::Healer), EnemyBehavior::makeEnemy(EnemyKind::Grunt)};
    for (auto& enemy : enemies) enemy.distance = 100.f;
    enemies[1].health = 1;
    std::vector<Shot> effects;
    constexpr float tick = 1.f / 60.f;
    EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, tick, effects);
    require(enemies[1].health == 13 && enemies[0].state == EnemyState::Healing, "Healing begins immediately and pauses movement");
    float elapsed = 0.f;
    while (enemies[0].state == EnemyState::Healing && elapsed < 1.f) {
        EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, tick, effects);
        elapsed += tick;
    }
    require(enemies[0].state == EnemyState::Marching && EnemyBehavior::movementSpeed(enemies[0]) == 70.f,
            "Healer resumes marching after its cast");
    require(elapsed >= EnemyBehavior::HEAL_CAST_TIME - tick && elapsed <= EnemyBehavior::HEAL_CAST_TIME + tick * 2,
            "Healing cast duration must match configuration");
    require(enemies[1].health == 13 && enemies[0].abilityCooldown > 0.f, "Resuming movement must not bypass heal cooldown");
    while (effects.size() == 1 && elapsed < 3.f) {
        EnemyBehavior::update(0, enemies, {}, map, Game::RANGE, tick, effects);
        elapsed += tick;
    }
    require(effects.size() == 2 && enemies[1].health == 25, "Exactly one new heal after cooldown");
    require(elapsed >= EnemyBehavior::HEAL_COOLDOWN - tick && elapsed <= EnemyBehavior::HEAL_COOLDOWN + tick * 2,
            "Heals must respect the full cooldown");
}

int main(int argc, char** argv) {
    return runTests(argc, argv, {
        {"shield_facing", shields}, {"healer_triage", healers}, {"scout_coverage", scouts},
        {"targeting_and_mixed_wave", targetingAndWave}, {"scout_cooldown_cycle", scoutCooldownCycle},
        {"healer_cast_cycle", healerCastCycle}
    });
}
