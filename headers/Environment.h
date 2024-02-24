//
// Created by Dentair on 20.02.2024.
//

#ifndef BATTLECITY_REMASTERED_ENVIRONMENT_H
#define BATTLECITY_REMASTERED_ENVIRONMENT_H

#include <iostream>
#include "SDL.h"
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

    [[nodiscard]] int GetSpeed() const;

    [[nodiscard]] int GetColor() const;
    void SetColor(int color);

private:
    int _x{};
    int _y{};
    int _width{};
    int _height{};
    int _speed{4};
    int _color{};
};

class Pawn;

class Environment {
public:
    int             windowWidth = 640;
    int             windowHeight = 480;
    int*            windowBuffer{};
    SDL_Event		event{};
    SDL_Window*     window{};
    SDL_Renderer*	renderer{};
    SDL_Texture*	screen{};
    bool            isGameOver = false;

    void		SetPixel(int x, int y, int color)const;

    MouseButtons        mouseButtons;
    std::vector<Pawn*>  allPawns;
};

class IMovable
{
public:
    virtual ~IMovable() = default;
    virtual void Move(const Environment& env) = 0;
};

class IDrawable
{
public:
    virtual ~IDrawable() = default;
    virtual void Draw(Environment& env) const = 0;
};

class Pawn : public BaseObj, public IMovable, public IDrawable
{
public:
    Pawn(int x, int y, int width, int height, int color);

    virtual ~Pawn() = default;

    void Move(const Environment& env) override;

    void Draw(Environment& env) const override;

    virtual void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key);

    PlayerKeys keyboardButtons;

    bool IsCollideWith(const Pawn* other) const;

    bool IsCanMove(const Environment &env) const;
};


class PlayerOne final : public Pawn
{
public:
    PlayerOne(int x, int y, int width, int height, int color);

    void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;
};

class PlayerTwo final : public Pawn
{
public:
    PlayerTwo(int x, int y, int width, int height, int color);

    void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;
};

#endif //BATTLECITY_REMASTERED_ENVIRONMENT_H
