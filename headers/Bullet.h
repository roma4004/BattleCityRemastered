#pragma once
#include "../headers/Environment.h"


class Bullet final : public Pawn
{
public:
    Bullet(int x, int y, int width, int height, int color, int speed, Direction direction, int health);
    

    ~Bullet() override;

    void Move(Environment& env) override;
    void Draw(Environment& env) const override;
    void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;
    
    [[nodiscard]] int GetDamage() const;
    void SetDamage(const int damage);
    
    void Shot(Environment& env) override;

private:
    int _damage{15}; 
};