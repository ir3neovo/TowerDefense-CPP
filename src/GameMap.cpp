
#include "GameMap.hpp"

GameMap::GameMap() {
    // init with grass
    for (auto& row : tiles_) {
        row.fill(TileType::Grass);
    }

    for (int col = 0; col <= 6; ++col) {
        tiles_[4][col] = TileType::Path;
    }

    // downward turn
    for (int row = 4; row <= 10; ++row) {
        tiles_[row][6] = TileType::Path;
    }

    for (int col = 6; col <= 13; ++col) {
        tiles_[10][col] = TileType::Path;
    }

    // upward turn
    for (int row = 6; row <= 10; ++row) {
        tiles_[row][13] = TileType::Path;
    }

    // exit to right
    for (int col = 13; col < COLS; ++col) {
        tiles_[6][col] = TileType::Path;
    }
}

std::optional<sf::Vector2i> GameMap::pixelToCell(
    sf::Vector2i pixel
) const {
    if (pixel.x < 0 || pixel.y < 0 ||
        pixel.x >= COLS * TILE_SIZE ||
        pixel.y >= ROWS * TILE_SIZE) {
        return std::nullopt;
    }

    return sf::Vector2i{
        pixel.x / TILE_SIZE,
        pixel.y / TILE_SIZE
    };
}

bool GameMap::isBuildable(sf::Vector2i cell) const {
    if (cell.x < 0 || cell.x >= COLS ||
        cell.y < 0 || cell.y >= ROWS) {
        return false;
    }

    return tiles_[cell.y][cell.x] ==
           TileType::Grass;
}

void GameMap::draw(
    sf::RenderWindow& window,
    std::optional<sf::Vector2i> hovered
) const {
    sf::RectangleShape tile({
        static_cast<float>(TILE_SIZE - 1),
        static_cast<float>(TILE_SIZE - 1)
    });

    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            tile.setPosition({
                static_cast<float>(col * TILE_SIZE),
                static_cast<float>(row * TILE_SIZE)
            });

            bool isPath =
                tiles_[row][col] == TileType::Path;

            sf::Color color = isPath
                ? sf::Color(188, 153, 106)
                : sf::Color(70, 95, 75);

            if (hovered &&
                hovered->x == col &&
                hovered->y == row) {
                color = isPath
                    ? sf::Color(220, 175, 110)
                    : sf::Color(180, 210, 140);
            }

            tile.setFillColor(color);
            window.draw(tile);
        }
    }
}
