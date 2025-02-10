# BattleCityRemastered
Please note the game is in a development state so the latest version is on the "dev" branch.

Like my old, [Battle City on C++/QT QML](https://github.com/roma4004/battle_city_qt_qml) project but remastered as a mentoring project for studying new programmers
and for me to practice design patterns and game architecture in modern C++.

![зображення](https://github.com/roma4004/BattleCityRemastered/assets/16146920/74cca7c2-7497-441c-9652-66f47282291a)
Already done:
* moving (for both players on one keyboard),
* shooting with reload timer with reusing bullets instance,
* destroying tanks and obstacles, indestructible obstacles and water that can't be passed but bullets traverse above it,
* enemy's AI and coop AI,
* respawning tank and bonuses,
* scoring and detailed statistics,
* animated menu with game modes and pause,
* tank heath bar,
* pickupable bonuses (spawn every 60sec in random location) from the original game like so:
  * Grenade(destroy enemy team tank),
  * Helmet(temporary invincibility),
  * Shovel(temporarily turns brick walls around the fortress to stone and repair broken),
  * Star(increases tank power),
  * Tank(extra life),
  * Timer(freeze enemy team),
* all bonuses also can be pickupable for an enemy team with the opposite effect.

Used technologies:
* C++20,
* EventSystem,
* SDL2, SDL2_TTF, SDL2_Image, SDL2_Mixer
* google unit tests,
* NuGet dependencies for *.sln and lib source for CMake, so it can be compiled for both CMake (CLion) or *.sln project (Rider, Visual Studio)

Used design patterns: 
* event bus, 
* observer, 
* strategy, 
* IoC,
* Object Pool

TODO roadmap: 
* network multiplayer (in progress, ~80%)
* use SDL_Image for texture (in progress, one logo in menu),
* SDL_Mixer for sound (in progress, one start music),
* unique bonuses,
* patterns Lightweight for images,
* QuadTree for collision detection,
* Thread Pool even for faster "update" and "draw",
* ECS (entity component system)
* maybe rewrite for DOTs using
