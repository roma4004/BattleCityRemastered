#pragma once

#include "SDL.h"
//#include <SDL2/SDL_image.h>
//#include <SDL2/SDL_ttf.h>
#include <vector>
//#include <list>

#include "../headers/MouseButton.h"
#include "../headers/Pawn.h"

class Pawn;

class Environment {
public:
    int             windowWidth = 640;
    int             windowHeight = 480;
    int*            windowBuffer{};
    SDL_Event       event{};
    SDL_Window*     window{};
    SDL_Renderer*   renderer{};
    SDL_Texture*    screen{};
    bool            isGameOver = false;

    void            SetPixel(int x, int y, int color) const;

    MouseButtons        mouseButtons;
    std::vector<Pawn*>  allPawns;
    //std::list<Pawn*>    allPawns;
};