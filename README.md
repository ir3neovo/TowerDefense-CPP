# Tower Defense — First Stand

A playable C++17 / SFML 3.1 tower-defense prototype. Build a defense, survive one wave, and restart to try another layout.

## Play

- **Left click a grass tile:** buy a tower for 50 coins. The preview shows its range; red indicates an invalid purchase.
- **Space / Start Wave:** launch the 12-enemy wave. During combat, Space toggles pause.
- **P / Pause:** pause or resume. Losing window focus pauses combat automatically.
- **R / Restart:** reset the complete game, including towers, enemies, lives and coins.
- **Escape:** close the window.

You begin with **150 coins and 5 lives**. Each defeated enemy awards **15 coins**. Each enemy reaching the exit costs one life. Clear all 12 enemies with at least one life remaining to win; reaching zero lives ends the game. Towers may be purchased during combat, but not while paused or after a result.

Towers target the enemy furthest along the route within their 125-pixel range. They deal 20 damage every 0.6 seconds; enemies have 60 health and move at 70 pixels per second. Hits are immediate, with a short beam flash for feedback.

A starter layout uses grass just above the first horizontal path, above the lower horizontal path, and above the final horizontal path. There is intentionally one tower type and one wave in this first playable loop.

## Build and run

Requires CMake 3.28+, Git, a C++17 compiler, and an internet connection for the initial SFML/dependency download. On Windows, install Visual Studio Build Tools with the C++ desktop workload and Windows SDK.

```powershell
cmake -S . -B build -DSFML_BUILD_AUDIO=OFF -DSFML_BUILD_NETWORK=OFF -DBUILD_SHARED_LIBS=OFF
cmake --build build --config Release
.\build\bin\Release\main.exe
```

For single-configuration generators such as Ninja, use `-DCMAKE_BUILD_TYPE=Release`; the executable is normally `build/bin/main` instead. Linux needs SFML's development dependencies (OpenGL, X11 extensions, udev, FreeType and HarfBuzz). See the [SFML CMake template](https://github.com/SFML/cmake-sfml-project) for platform setup.

All visual assets are drawn in code, including a small bitmap alphabet, so there are no font or image paths to configure.

## Structure

- `GameMap`: ordered, adjacent route cells; grass/path drawing; coordinate conversion; route interpolation.
- `Game`: wave spawning, placement rules, targeting, damage, economy, lives, pause, and results.
- `main.cpp`: input, fixed 60 Hz simulation updates, drawing, HUD and buttons.
- `PixelText`: embedded 5x7 bitmap text for the UI.

The map is generated from the same ordered route used for movement. Entry and exit are one cell outside the board; corners are included once. Enemies carry distance traveled, so movement follows each segment without skipping turns. The simulation uses a fixed timestep and limits catch-up after a stalled frame.

## Checks

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

The executable checks route continuity and corner positions, placement bounds, occupied tiles, insufficient currency, pause, a winning layout, the undefended loss case, reward accounting, and restart. Tests do not open a game window.

## Next possibilities

Multiple waves, tower upgrades, enemy types, sound, and saved scores can build on this loop. Dynamic pathfinding is unnecessary for the current fixed route.

## License

See [LICENSE.md](LICENSE.md). The project began from the SFML CMake template.
