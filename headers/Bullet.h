#pragma once

#include "../headers/Pawn.h"

struct Point;
class Environment;

class Bullet final : public Pawn
{
public:
	Bullet(const Point& pos, int width, int height, int color, int speed, Direction direction, int health);

	~Bullet() override;

	void Move(Environment& env) override;
	void Draw(Environment& env) const override;
	void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;

	[[nodiscard]] int GetDamage() const;
	void SetDamage(int damage);

	void Shot(Environment& env) override;

	std::tuple<bool, Pawn*> IsCanMove(const SDL_Rect* self, const Environment& env) const override;

private:
	int _damage{15};
	void DealDamage(Pawn* pawn);
};