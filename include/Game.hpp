#pragma once
#include "GameMap.hpp"
#include "Entities.hpp"
#include <vector>

enum class Phase { Planning, Running, Won, Lost };

class Game {
public:
    static constexpr int TOWER_COST = 50;
    static constexpr int STARTING_COINS = 150;
    static constexpr int STARTING_LIVES = 5;
    static constexpr int WAVE_SIZE = 12;
    static constexpr int KILL_REWARD = 15;
    static constexpr int DAMAGE = 20;
    static constexpr float RANGE = 125.f;
    static constexpr float FIRE_INTERVAL = 0.6f;
    static constexpr float ENEMY_SPEED = 70.f;
    static constexpr float SPAWN_INTERVAL = 0.9f;

    GameMap map;
    bool canPlace(sf::Vector2i cell) const;
    bool placeTower(sf::Vector2i cell);
    bool toggleTargeting(sf::Vector2i cell);
    void startWave();
    void togglePause();
    void reset();
    // Call with a fixed timestep; rendering never changes simulation state.
    void update(float seconds);

    Phase phase() const { return phase_; }
    bool paused() const { return paused_; }
    int coins() const { return coins_; }
    int lives() const { return lives_; }
    int spawned() const { return spawned_; }
    int defeated() const { return defeated_; }
    int escaped() const { return escaped_; }
    int blockedDamage() const { return blockedDamage_; }
    int healedHealth() const { return healedHealth_; }
    int scoutDashes() const { return scoutDashes_; }
    const std::vector<Enemy>& enemies() const { return enemies_; }
    const std::vector<Tower>& towers() const { return towers_; }
    const std::vector<Shot>& shots() const { return shots_; }

private:
    Phase phase_ = Phase::Planning;
    bool paused_ = false;
    int coins_ = STARTING_COINS, lives_ = STARTING_LIVES;
    int spawned_ = 0, defeated_ = 0, escaped_ = 0;
    int blockedDamage_ = 0, healedHealth_ = 0, scoutDashes_ = 0;
    float spawnTimer_ = 0.f;
    std::vector<Enemy> enemies_;
    std::vector<Tower> towers_;
    std::vector<Shot> shots_;
};
