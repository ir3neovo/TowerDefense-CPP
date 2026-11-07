#include "Game.hpp"
#include "EnemyBehavior.hpp"
#include <algorithm>
#include <array>

namespace {
constexpr std::array<EnemyKind, Game::WAVE_SIZE> WAVE = {
    EnemyKind::Grunt, EnemyKind::Shield, EnemyKind::Healer, EnemyKind::Scout,
    EnemyKind::Grunt, EnemyKind::Shield, EnemyKind::Healer, EnemyKind::Scout,
    EnemyKind::Grunt, EnemyKind::Shield, EnemyKind::Healer, EnemyKind::Scout
};
}

bool Game::canPlace(sf::Vector2i cell) const {
    if ((phase_ != Phase::Planning && phase_ != Phase::Running) || paused_
        || coins_ < TOWER_COST || !map.isBuildable(cell)) return false;
    return std::none_of(towers_.begin(), towers_.end(), [cell](const Tower& tower) { return tower.cell == cell; });
}

bool Game::placeTower(sf::Vector2i cell) {
    if (!canPlace(cell)) return false;
    towers_.push_back({cell, 0.f, Targeting::First});
    coins_ -= TOWER_COST;
    return true;
}

bool Game::toggleTargeting(sf::Vector2i cell) {
    if ((phase_ != Phase::Planning && phase_ != Phase::Running) || paused_) return false;
    for (auto& tower : towers_) if (tower.cell == cell) {
        tower.targeting = tower.targeting == Targeting::First ? Targeting::HealersFirst : Targeting::First;
        return true;
    }
    return false;
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
        enemies_.push_back(EnemyBehavior::makeEnemy(WAVE[spawned_]));
        ++spawned_;
        spawnTimer_ += SPAWN_INTERVAL;
    }
    // Abilities inspect a common position snapshot before anybody moves this tick.
    for (std::size_t i = 0; i < enemies_.size(); ++i) {
        const auto events = EnemyBehavior::update(i, enemies_, towers_, map, RANGE, seconds, shots_);
        healedHealth_ += events.healed;
        if (events.beganDash) ++scoutDashes_;
    }
    for (auto& enemy : enemies_) enemy.distance += EnemyBehavior::movementSpeed(enemy) * seconds;
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
        const auto index = EnemyBehavior::towerTarget(tower, enemies_, map, RANGE);
        if (!index) continue;
        auto& target = enemies_[*index];
        const auto center = GameMap::cellCenter(tower.cell);
        const int damage = EnemyBehavior::damageFrom(target, center, map, DAMAGE);
        const bool blocked = damage < DAMAGE;
        if (blocked) {
            target.state = EnemyState::Guarding;
            target.stateTime = 0.2f;
            blockedDamage_ += DAMAGE - damage;
        }
        shots_.push_back({center, map.positionAlongRoute(target.distance), 0.12f,
                          blocked ? ShotKind::Blocked : ShotKind::Damage});
        target.health -= damage;
        tower.cooldown = FIRE_INTERVAL;
        if (target.health <= 0) { ++defeated_; coins_ += KILL_REWARD; }
    }
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& enemy) {
        return enemy.health <= 0;
    }), enemies_.end());
    if (spawned_ == WAVE_SIZE && enemies_.empty()) phase_ = Phase::Won;
}
