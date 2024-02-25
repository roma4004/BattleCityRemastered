#include "../headers/PlayerTwo.h"
#include "../headers/pawn.h"

PlayerTwo::PlayerTwo(const int x, const int y, const int width, const int height, const int color, const int speed)
    : Pawn(x, y, width, height, color, speed) { }

void PlayerTwo::KeyboardEvensHandlers(Environment& env, const Uint32 eventType, const SDL_Keycode key)
{
    if  (eventType == SDL_KEYDOWN)
    {
        if (key == SDLK_LEFT)
        {
            keyboardButtons.A = true;
        }
        else if (key == SDLK_RIGHT)
        {
            keyboardButtons.D = true;
        }
        else if (key == SDLK_DOWN)
        {
            keyboardButtons.S = true;
        }
        else if (key == SDLK_UP)
        {
            keyboardButtons.W = true;
        }
    }
    else if  (eventType == SDL_KEYUP)
    {
        if (key == SDLK_LEFT)
        {
            keyboardButtons.A = false;
        }
        else if (key == SDLK_RIGHT)
        {
            keyboardButtons.D = false;
        }
        else if (key == SDLK_DOWN)
        {
            keyboardButtons.S = false;
        }
        else if (key == SDLK_UP)
        {
            keyboardButtons.W = false;
        }
    }
}