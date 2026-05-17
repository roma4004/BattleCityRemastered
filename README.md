# BattleCityRemastered

Please note the game is in a development state so the latest version is on the "dev" branch.

Like my old, [Battle City on C++/QT QML](https://github.com/roma4004/battle_city_qt_qml) project but remastered as a mentoring project for studying new programmers
and for me to practice design patterns and game architecture in modern C++. If you want to join or stydy with me, let me know in direct 
https://t.me/Dentair.

<img width="1201" height="952" alt="зображення" src="https://github.com/user-attachments/assets/054932d5-2ed3-4507-b071-13e56bcc49f9" />

Already done:
* moving (for both players on one keyboard, with gamepad support, tested on Xbox and PlayStation controllers)
* shooting with reload timer (reusing bullets instance),
* destroying tanks and obstacles, indestructible obstacles and water that can't be passed but bullets traverse above it,
* destroying bullet phisics obstacles, mean that bullet have explosion radius to cause AOE damage,
* tank heath bar,
* enemy's AI and coop AI,
* scoring and detailed statistics,
* animated menu with game modes and pause,
* respawning tank and bonuses,
* pickupable bonuses (spawn every 60sec in random location) from the original game like so:
  * Grenade(destroy enemy team tank),
  * Helmet(temporary invincibility),
  * Shovel(temporarily turns brick walls around the fortress to steel and repair destroyed parts),
  * Star(increases tank power, when pick up more than 2 can destroy steel walls),
  * Tank(extra life),
  * Timer(freeze enemy team),
  * TODO: bonus that allow you destroy obstacles while moveing, get bigger tank and bullet, ability limited by time
* all bonuses also can be pickupable for an enemy team with the opposite effect,
* network multiplayer (playing from two game sparated process, one start as host, other as client. )
* based on original textures and animation with some small difference and extendings
* end game (win\lose condition)
* separated score board
* controling keys hint

NOTE: For multiplayer game. 
Its two separated process that allow to trully play with full visual replication via TCP/IP. 
Game in host\client mode looks similar but with strict host authority, so clien can only do the key input.
  * (run game.exe twice) drag window, choose "play as host" in one window and "play as client" in other window
  * host will be on auto pause while awaiting client joining, so host should start first
  * Replicating all visual events (move, shot, died, spawn, statistics, animation)
  * Logic compute on host with acepting client inputs and partialy miroring back to client to create visual consequence 

Used technologies:
* C++20,
* EventSystem,
* SDL2, SDL2_TTF, SDL2_Image, SDL2_Mixer
* google unit tests (100+ tests),
* NuGet dependencies for *.sln and lib source for CMake, so it can be compiled for both CMake (CLion) or *.sln project (Rider, Visual Studio)

Used design patterns:
* Event Bus,
* Observer,
* Strategy,
* IoC,
* Object Pool
* Flyweight pattern

TODO roadmap:
* (in progress) side bar with respawn counters and level number (just like in oiginal)
* ice and bushes logic
* network multiplayer lobby system
* SDL_Mixer for sound (in progress, one start music),
* Entity Component System(in progress, have few behavior classses like component, more incoming and universal system planing),
* new unique bonuses,
* QuadTree for collision detection,
* Thread Pool even for faster "update" and "draw",
* maybe rewrite for DOTs using
