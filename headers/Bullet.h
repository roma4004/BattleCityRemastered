#pragma once
#include "../headers/Environment.h"


class Bullet final : public Pawn
{
public:
    Bullet(int x, int y, int width, int height, int color, int speed, Direction direction, size_t id);
    

    ~Bullet() override;

    void Move(Environment& env) override;
    void Draw(Environment& env) const override;
    void Explode(Environment& env);
    void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;
    
    [[nodiscard]] size_t GetId() const;
    
    void SetId(const size_t id);
    
    [[nodiscard]] int GetDamage() const;
    
    void SetDamage(const int damage);


private:
    size_t _id{};
    int _damage{};
};