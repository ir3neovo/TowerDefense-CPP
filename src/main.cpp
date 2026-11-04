#include <SFML/Graphics.hpp>
#include "GameMap.hpp"

int main()
{
	constexpr unsigned int WINDOW_WIDTH = 800;
    constexpr unsigned int WINDOW_HEIGHT = 600;
    constexpr int TILE_SIZE = 40;
	constexpr int ROWS = 15;
    constexpr int COLS = 20;


	sf::RenderWindow window(
        sf::VideoMode({WINDOW_WIDTH, WINDOW_HEIGHT}),
        "Tower Defense - Phase 1"
    );

    window.setFramerateLimit(60);
	// use the same shape to draw every tile in the grid, but change its position and color each time
    sf::RectangleShape tile({
        static_cast<float>(TILE_SIZE - 1),
        static_cast<float>(TILE_SIZE - 1)
    });

	GameMap gameMap;

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

		auto hovered = gameMap.pixelToCell(
            sf::Mouse::getPosition(window)
        );

        window.clear(sf::Color(30, 35, 40));

        gameMap.draw(window, hovered);
        window.display();
    }

    return 0;
}
