#pragma once

#include "../headers/Pawn.h"

struct Point;
struct Environment;

class Bullet final : public Pawn
{
public:
	Bullet(const Point& pos, int width, int height, int color, float speed, Direction direction, int health,
		   Environment* env);

	~Bullet() override;

	void Move() override;
	void Draw() const override;

	[[nodiscard]] int GetDamage() const;
	void SetDamage(int damage);

	void Shot() override;

	std::tuple<bool, std::list<BaseObj*>> IsCanMove(const BaseObj* me) override;
	void CheckAoE(const BaseObj* me, const Environment* env, std::list<BaseObj*>* aoeList) const;

private:
	void DealDamage(const std::list<BaseObj*>* objectList);

	int _damage{15};
};
