#include "../headers/Bullet.h"

#include <variant>



Bullet::Bullet(const int x, const int y, const int width, const int height, const int color, const int speed, Direction direction, const size_t id)
    : Pawn(x, y, width, height, color, speed, id)
{
    SetDirection(direction);
    SetId(id);
}


Bullet::~Bullet() = default;

void Bullet::Move(Environment& env)
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
            Explode(env);
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
            Explode(env);
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
            Explode(env);
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
            Explode(env);
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

size_t Bullet::GetId() const
{
    return _id;
}
void Bullet::SetId(const size_t id)
{
    _id = id;
}
int Bullet::GetDamage() const
{
    return _damage;
}
void Bullet::SetDamage(const int damage)
{
    _damage = damage;
}


void Bullet::Explode(Environment& env) 
{
    auto it = std::find(env.allPawns.begin(), env.allPawns.end(), GetId());
 
    if (it != env.allPawns.end())
    {
        env.allPawns.erase(it);
    }
    delete *it;
}
