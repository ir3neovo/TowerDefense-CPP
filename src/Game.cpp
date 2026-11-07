#include "Game.hpp"
#include <algorithm>

bool Game::canPlace(sf::Vector2i cell) const {
    if ((phase_ != Phase::Planning && phase_ != Phase::Running) || paused_
        || coins_ < TOWER_COST || !map.isBuildable(cell)) return false;
    return std::none_of(towers_.begin(), towers_.end(), [cell](const Tower& tower) { return tower.cell == cell; });
}

bool Game::placeTower(sf::Vector2i cell) {
    if (!canPlace(cell)) return false;
    towers_.push_back({cell, 0.f});
    coins_ -= TOWER_COST;
    return true;
}

void Game::startWave() {
    if (phase_ == Phase::Planning) phase_ = Phase::Running;
}

void Game::togglePause() {
    if (phase_ == Phase::Running) paused_ = !paused_;
}

void Game::reset() { *this = Game{}; }

void Game::update(float seconds) {
    if (phase_ != Phase::Running || paused_ || seconds <= 0.f) return;
    for (auto& shot : shots_) shot.remaining -= seconds;
    shots_.erase(std::remove_if(shots_.begin(), shots_.end(), [](const Shot& shot) {
        return shot.remaining <= 0.f;
    }), shots_.end());

    spawnTimer_ -= seconds;
    while (spawned_ < WAVE_SIZE && spawnTimer_ <= 0.f) {
        enemies_.push_back({});
        ++spawned_;
        spawnTimer_ += SPAWN_INTERVAL;
    }
    for (auto& enemy : enemies_) enemy.distance += ENEMY_SPEED * seconds;
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [this](const Enemy& enemy) {
        if (enemy.distance < map.routeLength()) return false;
        ++escaped_;
        lives_ = std::max(0, lives_ - 1);
        return true;
    }), enemies_.end());
    if (lives_ == 0) { phase_ = Phase::Lost; return; }

    for (auto& tower : towers_) {
        tower.cooldown = std::max(0.f, tower.cooldown - seconds);
        if (tower.cooldown > 0.f) continue;
        Enemy* target = nullptr;
        const auto center = GameMap::cellCenter(tower.cell);
        for (auto& enemy : enemies_) {
            if (enemy.health <= 0) continue;
            const auto offset = map.positionAlongRoute(enemy.distance) - center;
            if (offset.x * offset.x + offset.y * offset.y <= RANGE * RANGE
                && (!target || enemy.distance > target->distance)) target = &enemy;
        }
        if (!target) continue;
        shots_.push_back({center, map.positionAlongRoute(target->distance), 0.12f});
        target->health -= DAMAGE;
        tower.cooldown = FIRE_INTERVAL;
        if (target->health <= 0) { ++defeated_; coins_ += KILL_REWARD; }
    }
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& enemy) {
        return enemy.health <= 0;
    }), enemies_.end());
    if (spawned_ == WAVE_SIZE && enemies_.empty()) phase_ = Phase::Won;
}
