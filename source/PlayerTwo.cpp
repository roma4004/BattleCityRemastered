#include "../headers/PlayerTwo.h"
#include "../headers/pawn.h"

PlayerTwo::PlayerTwo(const int x, const int y, const int width, const int height, const int color, const int speed, const size_t id)
    : Pawn(x, y, width, height, color, speed, id) { }

void PlayerTwo::KeyboardEvensHandlers(Environment& env, const Uint32 eventType, const SDL_Keycode key)
{
    if  (eventType == SDL_KEYDOWN)
    {
        if (key == SDLK_LEFT)
        {
            keyboardButtons.a = true;
            SetDirection(Direction::LEFT);
        }
        else if (key == SDLK_RIGHT)
        {
            keyboardButtons.d = true;
            SetDirection(Direction::RIGHT);
        }
        else if (key == SDLK_DOWN)
        {
            keyboardButtons.s = true;
            SetDirection(Direction::DOWN);
        }
        else if (key == SDLK_UP)
        {
            keyboardButtons.w = true;
            SetDirection(Direction::UP);
        }
    }
    else if  (eventType == SDL_KEYUP)
    {
        if (key == SDLK_LEFT)
        {
            keyboardButtons.a = false;
        }
        else if (key == SDLK_RIGHT)
        {
            keyboardButtons.d = false;
        }
        else if (key == SDLK_DOWN)
        {
            keyboardButtons.s = false;
        }
        else if (key == SDLK_UP)
        {
            keyboardButtons.w = false;
        }
        else if (key == SDLK_RCTRL)
        {
            keyboardButtons.shot = true;
        }
    }
}