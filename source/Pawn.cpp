#include "../headers/Pawn.h"
#include <iostream>

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

bool Pawn::IsCollideWith(const SDL_Rect* self,const Pawn* other) const {
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
            std::cout << "Pawn::IsCanMove returned false" << '\n';

            return false;
        }
    }

    std::cout << "Pawn::IsCanMove returned true" << '\n';

    return true;
}

void Pawn::Move(const Environment& env) {
    const int speed = GetSpeed();
    if (keyboardButtons.A && GetX() + speed >= 0)
    {
        const auto self = SDL_Rect{ this->GetX() - this->GetSpeed(), this->GetY(), this->GetWidth(), this->GetHeight() };
        if (IsCanMove(&self, env)) {
            MoveX(-speed);
        }
    }

    if (keyboardButtons.D && GetX() + speed + GetWidth() < env.windowWidth)
    {
        const auto self = SDL_Rect{ this->GetX() + this->GetSpeed(), this->GetY(), this->GetWidth(), this->GetHeight() };
        if (IsCanMove(&self, env)) {
            MoveX(speed);
        }
    }

    if (keyboardButtons.W && GetY() + speed >= 0)
    {
        const auto self = SDL_Rect{ this->GetX(), this->GetY() - this->GetSpeed(), this->GetWidth(), this->GetHeight() };
        if (IsCanMove(&self, env)) {
            MoveY(-speed);
        }
    }

    if (keyboardButtons.S && GetY() + speed + GetHeight() < env.windowHeight)
    {
        const auto self = SDL_Rect{ this->GetX(), this->GetY() + this->GetSpeed(), this->GetWidth(), this->GetHeight() };
        if (IsCanMove(&self, env)) {
            MoveY(speed);
        }
    }
}

void Pawn::KeyboardEvensHandlers(Environment &env, Uint32 eventType, SDL_Keycode key) {

}

Pawn::Pawn(const int x, const int y, const int width, const int height, const int color, const int speed)
        : BaseObj(x, y, width, height, color, speed) {
}

Pawn::~Pawn() = default;