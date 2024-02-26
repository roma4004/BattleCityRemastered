#pragma once
#include "../headers/Environment.h"


class Bullet final : public Pawn
{
public:
    Bullet(int x, int y, int width, int height, int color, int speed, Direction direction);
    
    ~Bullet() override;

    void Move(const Environment& env) override;
    void Draw(Environment& env) const override;
    void Explode();
    void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;

private:
    int _bulletId{};
    int _bulletDamage{};
    int _bulletSpeed{};
};