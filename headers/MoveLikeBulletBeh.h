#pragma once

#include "BaseObj.h"
#include "Circle.h"
#include "Direction.h"
#include "MoveBeh.h"

#include <functional>
#include <memory>

class MoveLikeBulletBeh : public MoveBeh
{
	UPoint _windowSize{0, 0};
	BaseObj* _selfParent{nullptr};
	Direction _direction{UP};
	float _speed{0.f};
	std::vector<std::shared_ptr<BaseObj>>* _allPawns;
	int _damage{0};

public:
	MoveLikeBulletBeh(UPoint windowSize, float speed, int damage, BaseObj* selfParent,
	                  std::vector<std::shared_ptr<BaseObj>>* allPawns);

	~MoveLikeBulletBeh() override = default;

	static bool IsCollideWith(const Rectangle& r1, const Rectangle& r2);
	[[nodiscard]] std::list<std::weak_ptr<BaseObj>> IsCanMove(float deltaTime) const override;
	
	void MoveLeft(float deltaTime) const override;
	void MoveRight(float deltaTime) const override;
	void MoveUp(float deltaTime) const override;
	void MoveDown(float deltaTime) const override;

	void CheckCircleAoE(FPoint blowCenter, std::list<std::weak_ptr<BaseObj>>& aoeList) const;
	void DealDamage(const std::list<std::weak_ptr<BaseObj>>& objectList) const;

	[[nodiscard]] Direction GetDirection() const override { return _direction; }
	void SetDirection(const Direction direction) override { _direction = direction; }

	[[nodiscard]] float GetSpeed() const override { return _speed; }
};