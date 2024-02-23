//
// Created by Dentair on 20.02.2024.
//

#ifndef BATTLECITY_REMASTERED_ENVIRONMENT_H
#define BATTLECITY_REMASTERED_ENVIRONMENT_H

#include <iostream>
#include <SDL.h>
//#include <SDL2/SDL_image.h>
//#include <SDL2/SDL_ttf.h>
#include <cstdio>
#include <vector>

struct MouseButtons
{
    bool MouseLeftButton = false;
    bool MouseMiddleButton = false;
    bool MouseRightButton = false;
};

struct PlayerKeys
{
    bool W = false;
    bool A = false;
    bool S = false;
    bool D = false;
};


class Environment {
public:
    int             windowWidth = 640;
    int             windowHeight = 480;
    int*            windowBuffer{};
    SDL_Event		event{};
    SDL_Window*     window{};
    SDL_Renderer*	renderer{};
    SDL_Texture*	screen{};
    bool isGameOver = false;

    void		SetPixel(const int x, const int y, const int color)const;

    MouseButtons mouseButtons;
};

class BaseObj
{
public:
    BaseObj();
    BaseObj(int x, int y, int width, int height, int color);

    virtual ~BaseObj();

    [[nodiscard]] int getX() const;
    void setX(int x);

    [[nodiscard]] int getY() const;
    void setY(int y);

    [[nodiscard]] int getWidth() const;
    void setWidth(int width);

    [[nodiscard]] int getHeight() const;
    void setHeight(int height);

    void moveX(int i);

    void moveY(int i);

    int GetSpeed() const;

    int GetColor() const;

    void SetColor(const int color);

private:
    int _x{};
    int _y{};
    int _width{};
    int _height{};
    int _speed{4};
    int _color{};
};

class Square : public BaseObj
{
public:
    Square(const int x, const int y, const int width, const int height, const int color)
        : BaseObj(x, y, width, height, color)
    {
    }

    void    DrawSquareObject(Environment& env)const;
    void    RectangleMove(const Environment& env, const PlayerKeys& keys);

};

class IPawn
{
public:
    virtual ~IPawn() = default;
    virtual void KeyboardEvens(Environment& env, Uint32 eventType, SDL_Keycode k) = 0;
};

class PlayerOne final : public Square, IPawn
{
public:
    PlayerOne(int x, int y, int width, int height, int color);

     void KeyboardEvens(Environment& env, Uint32 eventType, SDL_Keycode key) override;

    PlayerKeys keyboardButtons;
};

class PlayerTwo final : public Square, IPawn
{
public:
    PlayerTwo(int x, int y, int width, int height, int color);

    void KeyboardEvens(Environment& env, Uint32 eventType, SDL_Keycode key) override;

    PlayerKeys keyboardButtons;
};

#endif //BATTLECITY_REMASTERED_ENVIRONMENT_H
