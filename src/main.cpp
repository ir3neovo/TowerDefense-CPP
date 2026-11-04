#include <SFML/Graphics.hpp>

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

    while (window.isOpen())
    {
        while (const auto event = window.pollEvent())
        {
            if (event->is<sf::Event::Closed>())
            {
                window.close();
            }
        }

		// get mouse position relative to the window.
        sf::Vector2i mousePos = sf::Mouse::getPosition(window);

        // convert pixel coord to grid coord
        int hoveredCol = mousePos.x / TILE_SIZE;
        int hoveredRow = mousePos.y / TILE_SIZE;

        // check whether the mouse is inside the grid
        bool mouseInside =
            mousePos.x >= 0 &&
            mousePos.y >= 0 &&
            mousePos.x < static_cast<int>(WINDOW_WIDTH) &&
            mousePos.y < static_cast<int>(WINDOW_HEIGHT);

        window.clear(sf::Color(30, 35, 40));

        // Draw a 20 x 15 tile grid.
        for (int row = 0; row < 15; ++row)
        {
            for (int col = 0; col < 20; ++col)
            {
                tile.setPosition({
                    static_cast<float>(col * TILE_SIZE),
                    static_cast<float>(row * TILE_SIZE)
                });

                // Highlight the tile under the mouse.
                if (mouseInside &&
                    row == hoveredRow &&
                    col == hoveredCol)
                {
                    tile.setFillColor(
                        sf::Color(180, 210, 140)
                    );
                }
                else
                {
                    tile.setFillColor(
                        sf::Color(70, 95, 75)
                    );
                }
                window.draw(tile);
            }
        }

        window.display();
    }

    return 0;
}
