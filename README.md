# BattleCityRemastered

Please note the game is in a development state so the latest version is on the "dev" branch.

Like my old, [Battle City on C++/QT QML](https://github.com/roma4004/battle_city_qt_qml) project but remastered as a mentoring project 
for studying new programmers. 
For me ins place to practice design patterns, game architecture in modern C++. 
If you want to join or stydy with me, let me know in direct [Telegram/Dentair](https://t.me/Dentair).

<img width="1201" height="948" alt="зображення" src="https://github.com/user-attachments/assets/cf1f9eea-8de7-4c58-b613-dd65f37728bc" />

example of mutiplayer in two separated process, each controll one player (host is left with yellow player, and client right control green player)
<img width="2405" height="949" alt="зображення" src="https://github.com/user-attachments/assets/24a10ff6-6629-47fc-af17-dcf0f26fc01d" />
and game over example in multiplayer, with score board
<img width="2402" height="950" alt="зображення" src="https://github.com/user-attachments/assets/f1b888f9-a6da-4132-b2d9-87f290be8c5f" />

Additional features or extended compare to the original title:
1) Destruction model, circle AOE damage from bullet touch point
2) Destroying bullet phisics obstacles, mean that bullet have explosion radius to cause AOE damage,
3) Bullet damage randomized in range,
4) Tank heath bar(with color gradation),
5) Bonus spawn only on free spots
6) Bonus can be destroyed
7) Enemy and Coop bots can pickup bonus
8) Coop AI game mode second player will be ally bot, thay will not shoot to fortress and player, but shoot other walls and enemies
9) Network multiplayer (playing from two game process, one will start as host, other as client). Hotseat also available from local game.
10) swaping controls of players (gamePads included), allow you to use playerTwo control scheme for playing playerOne (TAB key)
11) Gamepads support, tested on Xbox and PlayStation controllers)
12) Controling keys hint
13) Parsing console argument to run host process and client process at the same time, with shifted window position and network game mode started 
14) save\load settings into config INI-file, with default creating INI-file if not exist
15) In bushes tank hide their health bar
16) Mouse support, GameMode selecting\start level
17) Extended ice logic, tanks on it have the velocity so they can drift even diagonally

Already reimplement(from scratch) features from original game title:
1) Moving (for both players in one keyboard)
2) Shooting with reload timer (reusing bullets instance),
3) Destroying tanks/obstacles, indestructible obstacles and water that can't be passed but bullets traverse above it,
3) Bot AI (Enemy\Coop),
4) Animated menu with game modes selecting\start level from keyboard,
5) In game pause,
6) Respawning tank and bonuses,
7) Pickupable bonuses, for player and AI bots,
8) Enemy can pickup bonus with opposite effect(inverse team logic) or self apply positive effect.
9) Random bonus spawn each 60 sec, in random and free location,
10) Bonus variations from the original game like:
  * Grenade (destroy opponents team tank),
  * Helmet (temporary invincibility, for 15 sec),
  * Shovel (temporary turns brick walls around the fortress to SteelWall and repair destroyed parts, then after 15 sec, they turns into brickWall. In case enemy pickup just destroy fortressWalls),
  * Star (increases tank power, when pick up more than 2 can destroy steel walls),
  * Tank (extra life),
  * Timer (freeze enemy team, for 15 sec)
11) Intro music
12) Textures and animation based on original, with some small difference and extending
13) Animation: tanks spawn, BonusHelmet effect, bullet explosion, tank explosion 
14) End game (win\lose condition) with score board (statistics)
15) Right side bar with re-spawn counters and level number (just like in original)
16) Auto-activating BonusHelmet for 5 second for each tank on its spawn
17) Animation of Helmet bonus on tank (health bar also hide to visualize it)
18) Bushes logic that hide tanks from his opponents
19) Ice logic, tanks on it temporary continues driving after move input release


NOTE: For multiplayer game. 
Its two separated process that allow to trully play with full visual replication via TCP/IP. 
Game in host\client mode looks similar but with strict host authority, so clien can only do the key input.
  * (run game.exe twice) drag window, choose "play as host" in one window and "play as client" in other window
  * Host will be on auto pause while awaiting client joining
  * Replicating all visual events (move, shot, died, spawn, statistics, animation)
  * Logic compute on host with acepting client inputs and partialy miroring back to client to create visual consequence 
Hint: you can create BAT file, put there "game.exe -host && game.exe -client" and execute it, this will run both window at the same time, with offset, so windows will not cover each other.

Used technologies:
* C++20,
* EventSystem,
* SDL2, SDL2_TTF, SDL2_Image, SDL2_Mixer,
* Boost (serialization, asio, property_tree/ini),
* Google unit tests (170+ tests),
* NuGet dependencies for *.sln and lib source for CMake, so it can be compiled for both CMake (CLion) or *.sln project (Rider, Visual Studio)

Used design patterns:
* Event Bus,
* Observer,
* Strategy,
* IoC,
* Object Pool,
* Flyweight pattern

TODO roadmap:
* (in progress) settings screen, that alow you control volume of sound and music, enable vSync and other stuff,
* (in progress) Mouse support in settings screen
* parsing console argument to set window position and size,
* SDL_Mixer for sound (only start music ready),
* visualize time left for timers like bonusHelmet and bonusTimer,
* new unique bonuses:
  at least bonus that allow you destroy obstacles while moveing, get bigger tank and bullet, ability limited by time,
  at least bonus ship from some version of original game that alow you cross rivers,
* lobby system for network multiplayer,
* 4 players support in multiplayer,
* QuadTree for collision detection (yea it will be overkill, but cool for stydy),
* Thread Pool for faster "update" and "draw",
* maybe rewrite for DOTs using
