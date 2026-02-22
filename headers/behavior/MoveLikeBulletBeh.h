#pragma once

#include "Point.h"
#include "interfaces/IMoveBeh.h"
#include <boost/uuid/uuid.hpp>
#include <functional>
#include <memory>

// enum class Direction : char8_t;
class Bullet;
struct FPoint;
struct ObjRectangle;
class EventSystem;

class MoveLikeBulletBeh final : public IMoveBeh
{
	using buuid = boost::uuids::uuid;

	buuid& _uuid;
	ObjRectangle& _rect;
	Direction& _direction;
	float& _speed;
	double& _bulletDamageRadius;
	UPoint& _windowSize;
	std::vector<std::shared_ptr<BaseObj>>& _bulletTargets;

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};


	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>> GetCircleCollisionObjects(FPoint blowCenter) const;
	[[nodiscard]] bool IsCanMove(double deltaTime) const override;

	[[nodiscard]] bool Move(double deltaTime) override;
	[[nodiscard]] bool MoveLeft(double deltaTime) override;
	[[nodiscard]] bool MoveRight(double deltaTime) override;
	[[nodiscard]] bool MoveUp(double deltaTime) override;
	[[nodiscard]] bool MoveDown(double deltaTime) override;

	[[nodiscard]] ObjRectangle GetBulletPathRect(double deltaTime) const;
	[[nodiscard]] FPoint GetBulletNextPoint(double deltaTime) const;

public:
	MoveLikeBulletBeh(ObjRectangle& rect, Direction& dir, float& speed, buuid& uuid, double& damageRadius,
					  UPoint& windowSize, std::vector<std::shared_ptr<BaseObj>>& bulletTargets,
					  std::vector<std::shared_ptr<BaseObj>>* allObjects);

	~MoveLikeBulletBeh() override = default;

	[[nodiscard]] std::vector<Direction> GetFreePathSides(double deltaTime) const override;
};
