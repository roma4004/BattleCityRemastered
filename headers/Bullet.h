#pragma once

#include "../headers/Circle.h"
#include "../headers/Pawn.h"

struct FPoint;

class Bullet final : public Pawn
{
public:
	Bullet(const FPoint& pos, float width, float height, int color, float speed, Direction direction, int health,
		   int* windowBuffer, size_t windowWidth, size_t windowHeight, std::vector<std::shared_ptr<BaseObj>>* allPawns,
		   std::shared_ptr<EventSystem> events);

	~Bullet() override;

	void Move(float deltaTime) override;
	void Draw() const override;

	[[nodiscard]] int GetDamage() const;
	void SetDamage(int damage);
	static bool CheckIntersection(const Circle& circle, const Rectangle& rect);

	void Shot() override;

	std::list<std::weak_ptr<BaseObj>> IsCanMove(const float deltaTime) override;
	void CheckCircleAoE(FPoint blowCenter, std::list<std::weak_ptr<BaseObj>>& aoeList) const;
	void CheckAoE(float deltaTime, std::list<std::weak_ptr<BaseObj>>& aoeList) const;

private:
	void DealDamage(const std::list<std::weak_ptr<BaseObj>>& objectList);

	int _damage{15};
};
