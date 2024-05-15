#pragma once

#include "../headers/Pawn.h"

struct FPoint;
struct Environment;

class Bullet final : public Pawn
{
public:
	Bullet(const FPoint& pos, float width, float height, int color, float speed, Direction direction, int health,
		   Environment* env);

	~Bullet() override;

	void Move() override;
	void Draw() const override;

	[[nodiscard]] int GetDamage() const;
	void SetDamage(int damage);

	void Shot() override;

	std::list<std::weak_ptr<BaseObj>> IsCanMove() override;
	void CheckAoE(const BaseObj* me, const Environment* env, std::list<std::weak_ptr<BaseObj>>& aoeList) const;

private:
	void DealDamage(const std::list<std::weak_ptr<BaseObj>>& objectList);

	int _damage{15};
};
