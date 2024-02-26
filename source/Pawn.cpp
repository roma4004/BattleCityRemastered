#include "../headers/Pawn.h"
#include "../headers/Bullet.h"
#include <iostream>
#include <variant>

void Pawn::Draw(Environment& env) const
{
    for (int y = GetY(); y < GetY() + GetHeight(); y++)
    {
        for (int x = GetX(); x < GetX() + GetWidth(); x++)
        {
            env.SetPixel(x, y, GetColor());
        }
    }
}

bool Pawn::IsCollideWith(const SDL_Rect* self, const Pawn* other) const {
    if (this == other)
    {
        return false;
    }

    const auto rect2 = SDL_Rect{ other->GetX(), other->GetY(), other->GetWidth(), other->GetHeight() };
    SDL_Rect rect3;

    return SDL_IntersectRect(self, &rect2, &rect3);
}

bool Pawn::IsCanMove(const SDL_Rect* self,const Environment& env) const {
    for (const auto* pawn : env.allPawns)
    {
        if (IsCollideWith(self, pawn)) {
            return false;
        }
    }
    return true;
}

void Pawn::TickUpdate(Environment& env)
{
    Move(env);
    Shot(env);
}

void Pawn::Shot(Environment& env)
{
    if (keyboardButtons.shot)
    {
        //Bullet* projectile = new Bullet{320, 240, 10, 10, 0xffffff, 5, GetDirection(), env.allPawns.size() + 5000 };
        
        env.allPawns.emplace_back(new Bullet{320, 240, 10, 10, 0xffffff, 5, GetDirection(), env.allPawns.size() + 5000 });
        keyboardButtons.shot = false;
    }
}

void Pawn::Move(Environment& env) {
    const int speed = GetSpeed();
    if (keyboardButtons.a && GetX() + speed >= 0)
    {
        const auto self = SDL_Rect{ this->GetX() - this->GetSpeed(), this->GetY(), this->GetWidth(), this->GetHeight() };
        if (IsCanMove(&self, env)) {
            MoveX(-speed);
        }
    }

    if (keyboardButtons.d && GetX() + speed + GetWidth() < env.windowWidth)
    {
        const auto self = SDL_Rect{ this->GetX() + this->GetSpeed(), this->GetY(), this->GetWidth(), this->GetHeight() };
        if (IsCanMove(&self, env)) {
            MoveX(speed);
        }
    }

    if (keyboardButtons.w && GetY() + speed >= 0)
    {
        const auto self = SDL_Rect{ this->GetX(), this->GetY() - this->GetSpeed(), this->GetWidth(), this->GetHeight() };
        if (IsCanMove(&self, env)) {
            MoveY(-speed);
        }
    }

    if (keyboardButtons.s && GetY() + speed + GetHeight() < env.windowHeight)
    {
        const auto self = SDL_Rect{ this->GetX(), this->GetY() + this->GetSpeed(), this->GetWidth(), this->GetHeight() };
        if (IsCanMove(&self, env)) {
            MoveY(speed);
        }
    }
}

void Pawn::KeyboardEvensHandlers(Environment &env, Uint32 eventType, SDL_Keycode key) {

}

Pawn::Pawn(const int x, const int y, const int width, const int height, const int color, const int speed, const size_t id)
        : BaseObj(x, y, width, height, color, speed, id) {
}

Pawn::~Pawn() = default;

Direction Pawn::GetDirection() const
{
    return _direction;
}
void Pawn::SetDirection(const Direction direction)
{
    _direction = direction;
}
void Pawn::SetIsAlive(const bool isAlive)
{
    _isAlive = isAlive;
}
bool Pawn::GetIsAlive() const
{
    return _isAlive;
}

void Pawn::Destroy(Environment& env) const
{
    const auto it = std::find(env.allPawns.begin(), env.allPawns.end(), this);
    delete *it;
    env.allPawns.erase(it);
}
