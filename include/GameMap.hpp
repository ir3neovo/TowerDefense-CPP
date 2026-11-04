
#pragma once

#include <SFML/Graphics.hpp>
#include <array>
#include <optional>

class GameMap {
public:
    static constexpr int TILE_SIZE = 40;
    static constexpr int ROWS = 15;
    static constexpr int COLS = 20;

    GameMap();

    std::optional<sf::Vector2i> pixelToCell(
        sf::Vector2i pixel
    ) const;

    bool isBuildable(sf::Vector2i cell) const;

    void draw(
        sf::RenderWindow& window,
        std::optional<sf::Vector2i> hovered
    ) const;

private:
    enum class TileType {
        Grass,
        Path
    };

    std::array<
        std::array<TileType, COLS>,
        ROWS
    > tiles_{};
};
