# BattleCityRemastered
Please note the game is in a development state so the latest version is on the "dev" branch
Like my old Battle City but remastered as a mentoring project for studying new programmers

![зображення](https://github.com/roma4004/BattleCityRemastered/assets/16146920/10f3de05-6c19-4f89-a54b-19a7cb66d90e)

Used technologies:
C++20,
EventSystem,
SDL,
SDL_TTF,
google unit tests,
NuGet dependencies for *.sln and lib source for CMake, so it can be compiled for both CMake (CLion) or *.sln project (Rider, Visual Studio)

Used design patterns: 
event bus, 
observer, 
strategy, 
IoC

TODO: 
use SDL_Image for texture,
SDL_Mixer for sound,
QuadThree for collision detection,
thread pool even for faster "update" and "draw",
patterns lightweight for images and object pool for bullets
