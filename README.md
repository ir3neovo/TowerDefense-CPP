# Tower Defense — First Stand

A playable C++17 / SFML 3.1 tower-defense prototype. Build a defense, survive one wave, and restart to try another layout.

## Play

- **Left click a grass tile:** buy a tower for 50 coins. The preview shows its range; red indicates an invalid purchase.
- **Space / Start Wave:** launch the 12-enemy wave. During combat, Space toggles pause.
- **P / Pause:** pause or resume. Losing window focus pauses combat automatically.
- **R / Restart:** reset the complete game, including towers, enemies, lives and coins.
- **Right click an existing tower:** switch between First (furthest along the route) and Healers First. An H marks healer-priority towers; they fall back to First when no healer is in range.
- **H / Help:** open a short strategy guide. Simulation stops while help is open.
- **Escape:** close the window.

Begin with **150 coins and 5 lives**. Each defeated enemy awards **15 coins**. Each enemy reaching the exit costs one life. Clear all 12 enemies with at least one life remaining to win; reaching zero lives ends the game. Towers may be purchased during combat, but not while paused or after a result.

Towers have a 125-pixel range and deal 20 base damage every 0.6 seconds. Hits are immediate, with a short beam flash for feedback. Towers use First targeting by default; right-click changes their priority at no cost.

A starter layout uses grass just above the first horizontal path, above the lower horizontal path, and above the final horizontal path. There is one tower type and a short mixed wave: three repeating groups of Grunt, Shield, Healer, and Scout (12 enemies total).

## Enemies that react to your defense

| Type | Behavior | Counterplay |
|---|---|---|
| Red grunt | 60 HP, moves at 70 pixels/second | Standard targeting |
| Blue shield | 80 HP, moves at 60 pixels/second; absorbs 75% of damage arriving within its 120-degree forward arc | Attack from the side or rear; its shield turns with the route |
| Green healer | 45 HP; restores up to 12 HP to the most injured nearby ally every 1.6 seconds, stopping for 0.3 seconds when healing | Right-click towers to prioritize healers |
| Gold scout | 40 HP; scans 160 pixels ahead for weak coverage and marks it with an amber diamond | Overlap two tower ranges to cancel its warning or sprint |

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
- `Game`: mixed-wave spawning, placement rules, combat, economy, lives, pause, and results.
- `Entities`: enemy types/states, tower targeting modes, and visual effect data.
- `EnemyBehavior`: facing-based shield damage, radius queries for healing and targeting, coverage scans, and ability state transitions.
- `main.cpp`: input, fixed 60 Hz simulation updates, drawing, HUD and buttons.
- `PixelText`: embedded 5x7 bitmap text for the UI.

The map is generated from the same ordered route used for movement. Entry and exit are one cell outside the board; corners are included once. Enemies carry distance traveled, so movement follows each segment without skipping turns. The simulation uses a fixed timestep and limits catch-up after a stalled frame.

## Checks

```powershell
cmake --build build --config Release
ctest --test-dir build -C Release --output-on-failure
```

CTest runs two suites without opening a game window:

- `gameplay`: route continuity and interpolation, placement bounds, occupied tiles, insufficient currency, pause, a winning layout, undefended defeat, reward accounting, and restart.
- `enemy_behavior`: shield front/side/rear damage and turning, healing triage/range/cooldowns/health limits, scout warnings/sprints/coverage changes, healer-priority targeting with fallback, and integrated mixed-wave encounters.

## License

See [LICENSE.md](LICENSE.md). The project began from the SFML CMake template.
