#pragma once

#include "../headers/Pawn.h"
#include "../headers/Circle.h"

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
	static bool CheckIntersection(const Circle& circle, const Rectangle& rect);

	void Shot() override;

	std::list<std::weak_ptr<BaseObj>> IsCanMove() override;
	void CheckCircleAoE(const Environment* env, FPoint blowCenter, std::list<std::weak_ptr<BaseObj>>& aoeList) const;
	void CheckAoE(const Environment* env, std::list<std::weak_ptr<BaseObj>>& aoeList) const;

private:
	void DealDamage(const std::list<std::weak_ptr<BaseObj>>& objectList);

	int _damage{15};
};
