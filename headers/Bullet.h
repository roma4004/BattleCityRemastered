#pragma once

#include "../headers/Pawn.h"

struct Point;
struct Environment;

class Bullet final : public Pawn
{
public:
	Bullet(const Point& pos, int width, int height, int color, float speed, Direction direction, int health, Environment* env);

	~Bullet() override;

	void Move(Environment* env) override;
	void Draw(const Environment* env) const override;
	void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;

	[[nodiscard]] int GetDamage() const;
	void SetDamage(int damage);

	void Shot(Environment* env) override;

	std::tuple<bool,std::list<BaseObj*>> IsCanMove(const BaseObj* me, const Environment* env) override;
	void CheckAoE(const BaseObj* me, const Environment* env, std::list<BaseObj*>* aoeList) const;

private:
	void DealDamage(const std::list<BaseObj*>* objectList);

	int _damage{15};
};