//
// Created by Dentair on 22.02.2024.
//

#include "Environment.h"

BaseObj::BaseObj() = default;

BaseObj::BaseObj(const int x,const int y,const int width,const int height,const int color)
        : _x(x),  _y(y),  _width(width),  _height(height), _color(color)
{

}

BaseObj::~BaseObj() = default;

int BaseObj::getX() const {
    return _x;
}

void BaseObj::setX(const int x) {
    _x = x;
}

int BaseObj::getY() const {
    return _y;
}

void BaseObj::setY(const int y) {
    _y = y;
}

int BaseObj::getWidth() const {
    return _width;
}

void BaseObj::setWidth(const int width) {
    _width = width;
}

int BaseObj::getHeight() const {
    return _height;
}

void BaseObj::setHeight(const int height) {
    _height = height;
}
void BaseObj::moveX(const int i)
{
    _x += i;
}
void BaseObj::moveY(const int i)
{
    _y += i;
}
int BaseObj::GetSpeed() const
{
    return _speed;
}
int BaseObj::GetColor() const
{
    return _color;
}
void BaseObj::SetColor(const int color)
{
    _color = color;
}

void Environment::SetPixel(const int x, const int y, const int color) const
{
    if (x >= 0 && x < windowWidth && y >= 0 && y < windowHeight)
    {
        const int rowSize = windowWidth;
        windowBuffer[y * rowSize + x] = color;
    }
}

void Square::DrawSquareObject(Environment& env) const
{
    for (int y = getY(); y < getY() + getHeight(); y++)
    {
        for (int x = getX(); x < getX() + getWidth(); x++)
        {
            env.SetPixel(x, y, GetColor());
        }
    }
}

void Square::RectangleMove(const Environment& env, const PlayerKeys& keys)
{
    const int speed = GetSpeed();
    if (keys.A && getX() - speed >= 0)
    {
        moveX(-speed);
    }
    else if (keys.D && getX() + speed + getWidth() < env.windowWidth)
    {
        moveX(speed);
    }
    else if (keys.W && getY() - speed >= 0)
    {
        moveY(-speed);
    }
    else if (keys.S && getY() + speed + getHeight() < env.windowHeight)
    {
        moveY(speed);
    }
}

PlayerOne::PlayerOne(const int x, const int y, const int width, const int height, const int color)
    : Square(x, y, width, height, color) { }

void PlayerOne::KeyboardEvens(Environment& env, const Uint32 eventType, const SDL_Keycode key)
{
    if  (eventType == SDL_KEYDOWN)
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
    else if  (eventType == SDL_KEYUP)
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

PlayerTwo::PlayerTwo(const int x, const int y, const int width, const int height, const int color)
    : Square(x, y, width, height, color) { }

void PlayerTwo::KeyboardEvens(Environment& env, const Uint32 eventType, const SDL_Keycode key)
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
