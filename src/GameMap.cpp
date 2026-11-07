#include "GameMap.hpp"
#include <algorithm>
#include <cmath>

GameMap::GameMap() {
    for (auto& row : tiles_) row.fill(TileType::Grass);
    for (int col = -1; col <= 6; ++col) route_.push_back({col, 4});
    for (int row = 5; row <= 10; ++row) route_.push_back({6, row});
    for (int col = 7; col <= 13; ++col) route_.push_back({col, 10});
    for (int row = 9; row >= 6; --row) route_.push_back({13, row});
    for (int col = 14; col <= COLS; ++col) route_.push_back({col, 6});
    for (const auto cell : route_) {
        if (cell.x >= 0 && cell.x < COLS && cell.y >= 0 && cell.y < ROWS)
            tiles_[cell.y][cell.x] = TileType::Path;
    }
}

std::optional<sf::Vector2i> GameMap::pixelToCell(sf::Vector2i pixel) const {
    if (pixel.x < 0 || pixel.y < 0 || pixel.x >= COLS * TILE_SIZE || pixel.y >= ROWS * TILE_SIZE)
        return std::nullopt;
    return sf::Vector2i{pixel.x / TILE_SIZE, pixel.y / TILE_SIZE};
}

bool GameMap::isBuildable(sf::Vector2i cell) const {
    return cell.x >= 0 && cell.y >= 0 && cell.x < COLS && cell.y < ROWS
        && tiles_[cell.y][cell.x] == TileType::Grass;
}

sf::Vector2f GameMap::cellCenter(sf::Vector2i cell) {
    return {(cell.x + 0.5f) * TILE_SIZE, (cell.y + 0.5f) * TILE_SIZE};
}

float GameMap::routeLength() const {
    return static_cast<float>(route_.size() - 1) * TILE_SIZE;
}

sf::Vector2f GameMap::positionAlongRoute(float distance) const {
    const float step = std::clamp(distance, 0.f, routeLength()) / TILE_SIZE;
    const auto index = static_cast<std::size_t>(step);
    if (index >= route_.size() - 1) return cellCenter(route_.back());
    const auto from = cellCenter(route_[index]);
    return from + (cellCenter(route_[index + 1]) - from) * (step - static_cast<float>(index));
}

sf::Vector2f GameMap::directionAlongRoute(float distance) const {
    const auto step = static_cast<std::size_t>(std::clamp(distance, 0.f, routeLength()) / TILE_SIZE);
    const auto index = std::min(step, route_.size() - 2);
    const auto delta = route_[index + 1] - route_[index];
    return {static_cast<float>(delta.x), static_cast<float>(delta.y)};
}

void GameMap::draw(sf::RenderWindow& window, std::optional<sf::Vector2i> hovered) const {
    sf::RectangleShape tile({TILE_SIZE - 1.f, TILE_SIZE - 1.f});
    for (int row = 0; row < ROWS; ++row) {
        for (int col = 0; col < COLS; ++col) {
            tile.setPosition({static_cast<float>(col * TILE_SIZE), static_cast<float>(row * TILE_SIZE)});
            const bool path = tiles_[row][col] == TileType::Path;
            sf::Color color = path ? sf::Color(174, 147, 109)
                : ((row + col) % 2 ? sf::Color(57, 82, 67) : sf::Color(61, 87, 70));
            if (hovered && hovered->x == col && hovered->y == row)
                color = path ? sf::Color(203, 161, 115) : sf::Color(96, 125, 94);
            tile.setFillColor(color);
            window.draw(tile);
        }
    }
    // Dots expose the same ordered route used by the movement simulation.
    sf::CircleShape dot(2.f);
    dot.setOrigin({2.f, 2.f});
    dot.setFillColor(sf::Color(215, 192, 150));
    for (const auto cell : route_) {
        dot.setPosition(cellCenter(cell));
        window.draw(dot);
    }
}
