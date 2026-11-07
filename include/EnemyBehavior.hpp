#pragma once
#include "Entities.hpp"
#include "GameMap.hpp"
#include <optional>
#include <vector>

namespace EnemyBehavior {
constexpr float HEAL_RANGE = 115.f;
constexpr int HEAL_AMOUNT = 12;
constexpr float HEAL_COOLDOWN = 1.6f;
constexpr float HEAL_CAST_TIME = 0.3f;
constexpr float SCOUT_LOOKAHEAD = 160.f;
constexpr float SCOUT_WARNING = 0.6f;
constexpr float SCOUT_DASH_TIME = 1.2f;
constexpr float SCOUT_COOLDOWN = 2.4f;
constexpr float SCOUT_DASH_SPEED = 120.f;

struct Events { int healed = 0; bool beganDash = false; };
Enemy makeEnemy(EnemyKind kind);
int damageFrom(const Enemy& enemy, sf::Vector2f attacker, const GameMap& map, int baseDamage);
int coverageAt(float distance, const std::vector<Tower>& towers, const GameMap& map, float range);
float weakestPoint(float distance, const std::vector<Tower>& towers, const GameMap& map, float range);
std::optional<std::size_t> healingTarget(std::size_t healer, const std::vector<Enemy>& enemies, const GameMap& map);
std::optional<std::size_t> towerTarget(const Tower& tower, const std::vector<Enemy>& enemies, const GameMap& map, float range);
Events update(std::size_t index, std::vector<Enemy>& enemies, const std::vector<Tower>& towers,
              const GameMap& map, float towerRange, float seconds, std::vector<Shot>& effects);
float movementSpeed(const Enemy& enemy);
}
