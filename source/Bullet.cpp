#include "../headers/Bullet.h"

#include <variant>


Bullet::Bullet(const int x, const int y, const int width, const int height, const int color, const int speed, Direction direction)
    : Pawn(x, y, width, height, color, speed)
{
    SetDirection(direction);
}

Bullet::~Bullet() = default;

void Bullet::Move(const Environment& env)
{
    const int speed = GetSpeed();

    if (GetDirection() == UP && GetY() - GetSpeed() >= 0)
    {
        const auto self = SDL_Rect{ this->GetX(), this->GetY() - GetSpeed(), this->GetWidth(), this->GetHeight() };
        if (IsCanMove(&self, env))
        {
            MoveY(-speed);
        }
        else
        {
            Explode();
        }
    }
    else if (GetDirection() == DOWN && GetY() + GetSpeed() <= env.windowHeight)
    {
        const auto self = SDL_Rect{ this->GetX(), this->GetY() + GetSpeed(), this->GetWidth(), this->GetHeight() };
        if (IsCanMove(&self, env))
        {
            MoveY(speed);
        }
        else
        {
            Explode();
        }
    }
    else if (GetDirection() == LEFT && GetX() - GetSpeed() >= 0)
    {
        const auto self = SDL_Rect{ this->GetX() - this->GetSpeed(), this->GetY(), this->GetWidth(), this->GetHeight() };
        if (IsCanMove(&self, env))
        {
            MoveY(-speed);
        }
        else
        {
            Explode();
        }
    }
    else if (GetDirection() == RIGHT && GetX() + GetSpeed() <= env.windowWidth)
    {
        const auto self = SDL_Rect{ this->GetX() + this->GetSpeed(), this->GetY(), this->GetWidth(), this->GetHeight() };
        if (IsCanMove(&self, env))
        {
            MoveY(speed);
        }
        else
        {
            Explode();
        }
    }

    
}

void Bullet::Draw(Environment& env) const
{
    Pawn::Draw(env);
}

void Bullet::KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key)
{
    Pawn::KeyboardEvensHandlers(env, eventType, key);
}

void Bullet::Explode()
{
    
}
