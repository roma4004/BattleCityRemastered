# BattleCityRemastered
Please note the game is in a development state so the latest version is on the "dev" branch.

Like my old, [Battle City on C++/QT QML](https://github.com/roma4004/battle_city_qt_qml) project but remastered as a mentoring project for studying new programmers
and for me to practice design patterns and game architecture in modern C++.

![зображення](https://github.com/roma4004/BattleCityRemastered/assets/16146920/74cca7c2-7497-441c-9652-66f47282291a)
Already done moving (for both players on one keyboard), shooting with reload timer with reusing bullets instance, destroying tank and obstacle, enemyAI and coopAI, respawning, scoring, animated menu with game modes and pause, indestructible obstacle and water which can't be passed but bullets traverse above it, tank heath bar, detailed statistics.

Used technologies:
* C++20,
* EventSystem,
* SDL,
* SDL_TTF,
* google unit tests,
* NuGet dependencies for *.sln and lib source for CMake, so it can be compiled for both CMake (CLion) or *.sln project (Rider, Visual Studio)

Used design patterns: 
* event bus, 
* observer, 
* strategy, 
* IoC,
* Object Pool

TODO roadmap: 
* bonus system,
* unique bonuses,
* use SDL_Image for texture,
* patterns lightweight for images,
* SDL_Mixer for sound,
* QuadThree for collision detection,
* thread pool even for faster "update" and "draw",
* network multiplayer
