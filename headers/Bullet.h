#pragma once

#include "../headers/Pawn.h"

struct Point;
struct Environment;

class Bullet final : public Pawn
{
public:
	Bullet(const Point& pos, int width, int height, int color, int speed, Direction direction, int health, Environment* env);

	~Bullet() override;

	void Move(Environment* env) override;
	void Draw(const Environment* env) const override;
	void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;

	[[nodiscard]] int GetDamage() const;
	void SetDamage(int damage);

	void Shot(Environment* env) override;

	std::tuple<bool, BaseObj*> IsCanMove(const SDL_Rect* self, const Environment* env) const override;

private:
	void DealDamage(BaseObj* object);

	int _damage{15};
};