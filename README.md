# BattleCityRemastered

Please note the game is in a development state so the latest version is on the "dev" branch.

Like my old, [Battle City on C++/QT QML](https://github.com/roma4004/battle_city_qt_qml) project but remastered as a mentoring project 
for studying new programmers. 
For me ins place to practice design patterns, game architecture in modern C++. 
If you want to join or stydy with me, let me know in direct [Telegram/Dentair](https://t.me/Dentair).

<img width="1205" height="951" alt="зображення" src="https://github.com/user-attachments/assets/bfe7bc87-41e1-4ec2-80f0-5610c52088da" />

example of mutiplayer in two separated process, each controll one player (host is left with yellow player, and client right control green player)
<img width="2405" height="949" alt="зображення" src="https://github.com/user-attachments/assets/24a10ff6-6629-47fc-af17-dcf0f26fc01d" />
and game over example in multiplayer, with score board
<img width="2402" height="950" alt="зображення" src="https://github.com/user-attachments/assets/f1b888f9-a6da-4132-b2d9-87f290be8c5f" />


Already done:
* moving (for both players on one keyboard, with gamepad support, tested on Xbox and PlayStation controllers)
* shooting with reload timer (reusing bullets instance),
* destroying tanks/obstacles, indestructible obstacles and water that can't be passed but bullets traverse above it,
* destroying bullet phisics obstacles, mean that bullet have explosion radius to cause AOE damage,
* tank heath bar(with color gradation),
* enemy's AI and coop AI,
* animated menu with game modes and pause,
* respawning tank and bonuses,
* pickupable bonuses, even enemy can pickup bonus. Spawned every 60 sec in random location. Variations from the original game like:
  * Grenade (destroy enemy team tank),
  * Helmet (temporary invincibility, for 15 sec),
  * Shovel (temporarу turns brick walls around the fortress to steel and repair destroyed parts, for 15 sec),
  * Star (increases tank power, when pick up more than 2 can destroy steel walls),
  * Tank (extra life),
  * Timer (freeze enemy team, for 15 sec)
* network multiplayer (playing from two game process, one will start as host, other as client). Hotseat also available from local game.
* intro music
* textures and animation based on original, with some small difference and extendings
* animation: tankSpawn, BonusHelmet effect, bullet explosion, tank explosion 
* end game (win\lose condition) with score board (statistics)
* controling keys hint
* swaping controls of players (gamePads included), allow you to use playerTwo control scheme for playing playerOne (TAB key)
* side bar with respawn counters and level number (just like in oiginal)
* auto-activating bonusHelmet for 5 second for each tank on its spawn
* animation of Helmet bonus on tank (health bar also hide to visualize it)


NOTE: For multiplayer game. 
Its two separated process that allow to trully play with full visual replication via TCP/IP. 
Game in host\client mode looks similar but with strict host authority, so clien can only do the key input.
  * (run game.exe twice) drag window, choose "play as host" in one window and "play as client" in other window
  * Host will be on auto pause while awaiting client joining
  * Replicating all visual events (move, shot, died, spawn, statistics, animation)
  * Logic compute on host with acepting client inputs and partialy miroring back to client to create visual consequence 

Used technologies:
* C++20,
* EventSystem,
* SDL2, SDL2_TTF, SDL2_Image, SDL2_Mixer,
* Boost (serialization, asio, property_tree/ini),
* google unit tests (100+ tests),
* NuGet dependencies for *.sln and lib source for CMake, so it can be compiled for both CMake (CLion) or *.sln project (Rider, Visual Studio)

Used design patterns:
* Event Bus,
* Observer,
* Strategy,
* IoC,
* Object Pool,
* Flyweight pattern

TODO roadmap:
* (in progress) parsing console argument to run host process and client process at the same time, with setting window position,
* (in progress) save/load settings into config ini file,
* (in progress) SDL_Mixer for sound (only start music ready),
* visualize time left for timers like bonusHelmet and bonusTimer,
* settings screen, that alow you control volume of sound and music, enable vSync and other stuff,
* ice and bushes logic,
* new unique bonuses:
  at least bonus that allow you destroy obstacles while moveing, get bigger tank and bullet, ability limited by time,
  at least bonus ship from some version of original game that alow you cross rivers,
* network multiplayer lobby system,
* Entity Component System(in progress, have few behavior classses like component, more incoming and universal system planing),
* QuadTree for collision detection (yea it will be overkill),
* Thread Pool even for faster "update" and "draw",
* maybe rewrite for DOTs using
