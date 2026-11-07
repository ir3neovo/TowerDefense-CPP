#pragma once
#include <SFML/System/Vector2.hpp>

// Simulation data only; rendering reads these values without changing them.
enum class EnemyKind { Grunt, Shield, Healer, Scout };
enum class EnemyState { Marching, Guarding, Healing, Scouting, Warning, Dashing };
enum class Targeting { First, HealersFirst };
enum class ShotKind { Damage, Blocked, Heal };

struct Enemy {
    float distance = 0.f;
    int health = 60;
    int maxHealth = 60;
    EnemyKind kind = EnemyKind::Grunt;
    EnemyState state = EnemyState::Marching;
    float abilityCooldown = 0.f;
    float stateTime = 0.f;
    float weakPoint = -1.f;
};
struct Tower {
    sf::Vector2i cell;
    float cooldown = 0.f;
    Targeting targeting = Targeting::First;
};
struct Shot {
    sf::Vector2f from, to;
    float remaining = 0.12f;
    ShotKind kind = ShotKind::Damage;
};
