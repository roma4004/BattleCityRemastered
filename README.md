# BattleCityRemastered

Please note the game is in a development state, so the latest version is on the "dev" branch.

Like my old [Battle City on C++/QT QML](https://github.com/roma4004/battle_city_qt_qml) project, but remastered as a mentoring project 
for teaching new programmers. 
For me it is a place to practice design patterns and game architecture in modern C++. 
If you want to join or study with me, let me know via direct message [Telegram/Dentair](https://t.me/Dentair).

<img width="1201" height="948" alt="зображення" src="https://github.com/user-attachments/assets/cf1f9eea-8de7-4c58-b613-dd65f37728bc" />

example of multiplayer in two separate processes, each controlling one player (host on the left with the yellow player, client on the right with the green player)
<img width="2405" height="949" alt="зображення" src="https://github.com/user-attachments/assets/24a10ff6-6629-47fc-af17-dcf0f26fc01d" />
and a game over example in multiplayer, with the score board
<img width="2402" height="950" alt="зображення" src="https://github.com/user-attachments/assets/f1b888f9-a6da-4132-b2d9-87f290be8c5f" />

Additional or extended features compared to the original title:
1) Destruction model, circle AOE damage from the bullet touch point
2) Bullet physics for destroying obstacles, meaning a bullet has an explosion radius that causes AOE damage,
3) Bullet damage randomized in a range,
4) Tank health bar (with color gradation),
5) Bonuses spawn only on free spots
6) Bonuses can be destroyed
7) Enemy and Coop bots can pick up bonuses
8) In the Coop AI game mode the second player is an ally bot, it will not shoot at the fortress or the player, but will shoot other walls and enemies
9) Network multiplayer (playing from two game processes, one starts as host, the other as client). Hotseat is also available in a local game.
10) Swapping controls of players (gamepads included), lets you use the playerTwo control scheme to play playerOne (TAB key)
11) Gamepad support, tested on Xbox and PlayStation controllers
12) Control keys hint
13) Parsing console arguments to run the host process and the client process at the same time, with shifted window position, network game mode started,
    optional `skipintro` to keep the intro music playing from one window only, 
    optional `pos=X,Y` / `size=WIDTH,HEIGHT` to place and size the window - either one can be passed alone, whatever is left out keeps its INI value (an explicit position also disables the automatic monitor centering)
14) Save/load settings in a config INI file, creating a default INI file if it does not exist
15) Tanks in bushes hide their health bar
16) Mouse support, GameMode selection(mouse move) / level start (left mouse button)
17) Extended ice logic, tanks on it keep their velocity, so they can drift even diagonally

Already reimplemented (from scratch) features from the original game:
1) Moving (for both players on one keyboard)
2) Shooting with a reload timer (reusing bullet instances),
3) Destroying tanks/obstacles, indestructible obstacles and water that can't be passed but bullets traverse above it,
4) Bot AI (Enemy\Coop),
5) Animated menu with game mode selection / level start from the keyboard,
6) In game pause,
7) Respawning tanks and bonuses,
8) Pickupable bonuses, for the player and AI bots,
9) An enemy can pick up a bonus with the opposite effect (inverse team logic), or apply a positive effect to itself.
10) Random bonus spawn every 60 sec, in a random free location,
11) Bonus variations from the original game like:
  * Grenade (destroys the opposing team's tanks),
  * Helmet (temporary invincibility, for 15 sec),
  * Shovel (temporarily turns the brick walls around the fortress into SteelWall and repairs destroyed parts, then after 15 sec they turn back into brickWall. If an enemy picks it up, it just destroys the fortressWalls),
  * Star (increases tank power, picking up more than 2 lets it destroy steel walls),
  * Tank (extra life),
  * Timer (freezes the enemy team, for 15 sec)
12) Intro music
13) Textures and animation based on the original, with some small differences and extensions
14) Animation: tank spawn, BonusHelmet effect, bullet explosion, tank explosion 
15) End game (win\lose condition) with a score board (statistics)
16) Right side bar with respawn counters and level number (just like in the original)
17) Auto-activating BonusHelmet for 5 seconds for each tank on its spawn
18) Animation of the Helmet bonus on a tank (the health bar is hidden to visualize it)
19) Bushes logic that hides tanks from their opponents
20) Ice logic, tanks on it keep driving for a moment after the move input is released


NOTE: about the multiplayer game. 
It is two separate processes that let you truly play with full visual replication over TCP/IP. 
The game in host\client mode looks the same, but with strict host authority, so the client can only send key input.
  * (run game.exe twice) drag the window, choose "play as host" in one window and "play as client" in the other window
  * The host is auto paused while waiting for the client to join
  * Replicating all visual events (move, shot, died, spawn, statistics, animation)
  * Logic is computed on the host, which accepts client inputs and partially mirrors the result back to the client to create the visual consequence 
Hint: `scripts/run-host-and-client.bat`/`.sh` starts both windows at once, with offset. First argument - path to the exe, default is `cmake-build-debug-mingw/`.
`scripts/run-host-and-client-skipintro.*` - same, but the client skips the intro autoplay (sound stays on), so it doesn't play twice.
`scripts/run-host-and-client-tiled.*` - an example of the `pos`/`size` arguments: two 800x600 windows side by side.
  * CMake/CLion: targets `run_host_and_client` / `run_host_and_client_skipintro` / `run_host_and_client_tiled`
  * Visual Studio: run the script directly - VS can't start one exe twice

Used technologies:
* C++20,
* EventSystem,
* SDL2, SDL2_TTF, SDL2_Image, SDL2_Mixer,
* Boost (asio, uuid, property_tree/ini),
* ser20 (C++20 fork of cereal) for serialization,
* Google unit tests (180+ tests),
* git submodules shared by both build systems, so it can be compiled both as a CMake project (CLion) and as a *.sln project (Rider, Visual Studio)

Used design patterns:
* Event Bus,
* Observer,
* Strategy,
* IoC,
* Object Pool,
* Flyweight pattern

TODO roadmap:
* (in progress) settings screen that lets you control sound and music volume, enable vSync and other stuff,
* (in progress) mouse support in the settings screen,
* SDL_Mixer for sound (only the intro music is ready),
* visualize the time left for timers like bonusHelmet and bonusTimer,
* new unique bonuses:
  at least a bonus that lets you destroy obstacles while moving and makes the tank and bullet bigger, limited by time,
  at least the ship bonus from some versions of the original game, that lets you cross rivers,
* lobby system for network multiplayer,
* 4 players support in multiplayer,
* QuadTree for collision detection (yeah, it is overkill, but a nice thing to study),
* Thread Pool for faster "update" and "draw",
* maybe rewrite it data-oriented (DOTS style)
