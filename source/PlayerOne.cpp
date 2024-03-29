#include "../headers/PlayerOne.h"
#include "../headers/pawn.h"

PlayerOne::PlayerOne(const int x, const int y, const int width, const int height, const int color, const int speed, const size_t id)
    : Pawn(x, y, width, height, color, speed, id) { }

void PlayerOne::KeyboardEvensHandlers(Environment& env, const Uint32 eventType, const SDL_Keycode key)
{
    if (eventType == SDL_KEYDOWN)
    {
        if (key == SDLK_a)
        {
            keyboardButtons.a = true;
            SetDirection(Direction::LEFT);
        }
        else if (key == SDLK_d)
        {
            keyboardButtons.d = true;
            SetDirection(Direction::RIGHT);
        }
        else if (key == SDLK_s)
        {
            keyboardButtons.s = true;
            SetDirection(Direction::DOWN);
        }
        else if (key == SDLK_w)
        {
            keyboardButtons.w = true;
            SetDirection(Direction::UP);
        }
    }
    else if (eventType == SDL_KEYUP)
    {
        if (key == SDLK_a)
        {
            keyboardButtons.a = false;
        }
        else if (key == SDLK_d)
        {
            keyboardButtons.d = false;
        }
        else if (key == SDLK_s)
        {
            keyboardButtons.s = false;
        }
        else if (key == SDLK_w)
        {
            keyboardButtons.w = false;
        }
        else if (key == SDLK_SPACE)
        {
            keyboardButtons.shot = true;
        }
    }
}