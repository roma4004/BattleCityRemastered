#pragma once

#include "BaseObj.h"
#include "Direction.h"
#include "Point.h"
#include "interfaces/IMoveBeh.h"

#include <functional>
#include <memory>

class MoveLikeTankBeh : public IMoveBeh
{
	UPoint _windowSize{0, 0};
	BaseObj* _selfParent{nullptr};
	Direction _direction{UP};
	float _speed{0.f};
	std::vector<std::shared_ptr<BaseObj>>* _allPawns;

public:
	MoveLikeTankBeh(Direction direction, UPoint windowSize, float speed, BaseObj* selfParent,
	                std::vector<std::shared_ptr<BaseObj>>* allPawns);

	~MoveLikeTankBeh() override = default;

	static bool IsCollideWith(const Rectangle& r1, const Rectangle& r2);
	[[nodiscard]] std::list<std::weak_ptr<BaseObj>> IsCanMove(float deltaTime) const override;
	float FindMinDistance(const std::list<std::weak_ptr<BaseObj>>& pawns,
	                      const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const;

	void MoveLeft(float deltaTime) const override;
	void MoveRight(float deltaTime) const override;
	void MoveUp(float deltaTime) const override;
	void MoveDown(float deltaTime) const override;

	[[nodiscard]] Direction GetDirection() const override { return _direction; }
	void SetDirection(const Direction direction) override { _direction = direction; }

	[[nodiscard]] float GetSpeed() const override { return _speed; }
};
