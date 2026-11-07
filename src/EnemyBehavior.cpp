#include "EnemyBehavior.hpp"
#include <algorithm>
#include <cmath>

namespace {
float squaredDistance(sf::Vector2f a, sf::Vector2f b) {
    const auto offset = a - b;
    return offset.x * offset.x + offset.y * offset.y;
}
}
namespace EnemyBehavior {
Enemy makeEnemy(EnemyKind kind) {
    Enemy enemy;
    enemy.kind = kind;
    switch (kind) {
    case EnemyKind::Shield: enemy.maxHealth = 80; break;
    case EnemyKind::Healer: enemy.maxHealth = 45; break;
    case EnemyKind::Scout: enemy.maxHealth = 40; enemy.state = EnemyState::Scouting; break;
    default: break;
    }
    enemy.health = enemy.maxHealth;
    return enemy;
}

int damageFrom(const Enemy& enemy, sf::Vector2f attacker, const GameMap& map, int baseDamage) {
    if (baseDamage <= 0 || enemy.health <= 0) return 0;
    if (enemy.kind != EnemyKind::Shield) return baseDamage;
    const auto offset = attacker - map.positionAlongRoute(enemy.distance);
    const float length = std::sqrt(offset.x * offset.x + offset.y * offset.y);
    if (length < 0.001f) return baseDamage;
    const auto facing = map.directionAlongRoute(enemy.distance);
    const float alignment = (offset.x * facing.x + offset.y * facing.y) / length;
    // A 120-degree forward arc absorbs 75% of incoming damage.
    return alignment >= 0.5f ? std::max(1, baseDamage / 4) : baseDamage;
}

int coverageAt(float distance, const std::vector<Tower>& towers, const GameMap& map, float range) {
    const auto position = map.positionAlongRoute(distance);
    int count = 0;
    for (const auto& tower : towers)
        if (squaredDistance(position, GameMap::cellCenter(tower.cell)) <= range * range) ++count;
    return count;
}

float weakestPoint(float distance, const std::vector<Tower>& towers, const GameMap& map, float range) {
    float weakest = std::clamp(distance, 0.f, map.routeLength());
    int leastCoverage = coverageAt(weakest, towers, map, range);
    const float end = std::min(weakest + SCOUT_LOOKAHEAD, map.routeLength());
    for (float probe = weakest + 20.f; probe <= end; probe += 20.f) {
        const int coverage = coverageAt(probe, towers, map, range);
        if (coverage < leastCoverage) { weakest = probe; leastCoverage = coverage; }
    }
    return weakest;
}

std::optional<std::size_t> healingTarget(std::size_t healer, const std::vector<Enemy>& enemies, const GameMap& map) {
    if (healer >= enemies.size() || enemies[healer].health <= 0 || enemies[healer].kind != EnemyKind::Healer) return std::nullopt;
    std::optional<std::size_t> best;
    const auto origin = map.positionAlongRoute(enemies[healer].distance);
    for (std::size_t i = 0; i < enemies.size(); ++i) {
        const auto& ally = enemies[i];
        if (i == healer || ally.health <= 0 || ally.health >= ally.maxHealth || ally.distance >= map.routeLength()) continue;
        if (squaredDistance(origin, map.positionAlongRoute(ally.distance)) > HEAL_RANGE * HEAL_RANGE) continue;
        // Triage the lowest health fraction; stable order breaks ties.
        if (!best || ally.health * enemies[*best].maxHealth < enemies[*best].health * ally.maxHealth) best = i;
    }
    return best;
}

std::optional<std::size_t> towerTarget(const Tower& tower, const std::vector<Enemy>& enemies, const GameMap& map, float range) {
    std::optional<std::size_t> best;
    const auto origin = GameMap::cellCenter(tower.cell);
    for (std::size_t i = 0; i < enemies.size(); ++i) {
        const auto& enemy = enemies[i];
        if (enemy.health <= 0 || enemy.distance >= map.routeLength()) continue;
        if (squaredDistance(origin, map.positionAlongRoute(enemy.distance)) > range * range) continue;
        const int priority = tower.targeting == Targeting::HealersFirst && enemy.kind == EnemyKind::Healer ? 1 : 0;
        const int bestPriority = best && tower.targeting == Targeting::HealersFirst && enemies[*best].kind == EnemyKind::Healer ? 1 : 0;
        if (!best || priority > bestPriority || (priority == bestPriority && enemy.distance > enemies[*best].distance)) best = i;
    }
    return best;
}

Events update(std::size_t index, std::vector<Enemy>& enemies, const std::vector<Tower>& towers,
              const GameMap& map, float towerRange, float seconds, std::vector<Shot>& effects) {
    Events events;
    if (index >= enemies.size() || seconds <= 0.f) return events;
    auto& enemy = enemies[index];
    if (enemy.health <= 0 || enemy.distance >= map.routeLength()) return events;
    enemy.abilityCooldown = std::max(0.f, enemy.abilityCooldown - seconds);
    enemy.stateTime = std::max(0.f, enemy.stateTime - seconds);
    if (enemy.kind == EnemyKind::Shield && enemy.stateTime == 0.f) enemy.state = EnemyState::Marching;
    if (enemy.kind == EnemyKind::Healer) {
        if (enemy.state == EnemyState::Healing && enemy.stateTime > 0.f) return events;
        enemy.state = EnemyState::Marching;
        if (enemy.abilityCooldown > 0.f) return events;
        if (const auto target = healingTarget(index, enemies, map)) {
            auto& ally = enemies[*target];
            events.healed = std::min(HEAL_AMOUNT, ally.maxHealth - ally.health);
            ally.health += events.healed;
            enemy.state = EnemyState::Healing;
            enemy.stateTime = HEAL_CAST_TIME;
            enemy.abilityCooldown = HEAL_COOLDOWN;
            effects.push_back({map.positionAlongRoute(enemy.distance), map.positionAlongRoute(ally.distance), HEAL_CAST_TIME, ShotKind::Heal});
        }
    }
    if (enemy.kind == EnemyKind::Scout) {
        enemy.weakPoint = weakestPoint(enemy.distance, towers, map, towerRange);
        const bool exposed = coverageAt(enemy.distance, towers, map, towerRange) < 2;
        if (enemy.state == EnemyState::Dashing) {
            if (!exposed || enemy.stateTime == 0.f) {
                enemy.state = EnemyState::Scouting;
                enemy.abilityCooldown = SCOUT_COOLDOWN;
            }
        } else if (enemy.state == EnemyState::Warning) {
            if (!exposed) { enemy.state = EnemyState::Scouting; enemy.stateTime = 0.f; }
            else if (enemy.stateTime == 0.f) {
                enemy.state = EnemyState::Dashing;
                enemy.stateTime = SCOUT_DASH_TIME;
                events.beganDash = true;
            }
        } else if (exposed && enemy.abilityCooldown == 0.f) {
            enemy.state = EnemyState::Warning;
            enemy.stateTime = SCOUT_WARNING;
        }
    }
    return events;
}

float movementSpeed(const Enemy& enemy) {
    if (enemy.state == EnemyState::Healing) return 0.f;
    if (enemy.state == EnemyState::Dashing) return SCOUT_DASH_SPEED;
    return enemy.kind == EnemyKind::Shield ? 60.f : 70.f;
}
}
