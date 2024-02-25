#include "../headers/PlayerOne.h"
#include "../headers/pawn.h"

PlayerOne::PlayerOne(const int x, const int y, const int width, const int height, const int color, const int speed)
    : Pawn(x, y, width, height, color, speed) { }

void PlayerOne::KeyboardEvensHandlers(Environment& env, const Uint32 eventType, const SDL_Keycode key)
{
    if (eventType == SDL_KEYDOWN)
    {
        if (key == SDLK_a)
        {
            keyboardButtons.A = true;
        }
        else if (key == SDLK_d)
        {
            keyboardButtons.D = true;
        }
        else if (key == SDLK_s)
        {
            keyboardButtons.S = true;
        }
        else if (key == SDLK_w)
        {
            keyboardButtons.W = true;
        }
    }
    else if (eventType == SDL_KEYUP)
    {
        if (key == SDLK_a)
        {
            keyboardButtons.A = false;
        }
        else if (key == SDLK_d)
        {
            keyboardButtons.D = false;
        }
        else if (key == SDLK_s)
        {
            keyboardButtons.S = false;
        }
        else if (key == SDLK_w)
        {
            keyboardButtons.W = false;
        }
    }
}