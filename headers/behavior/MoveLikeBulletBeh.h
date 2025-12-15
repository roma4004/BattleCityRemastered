#pragma once

#include "Point.h"
#include "interfaces/IMoveBeh.h"
#include <functional>
#include <memory>
#include <boost/uuid/uuid.hpp>

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
	[[nodiscard]] bool IsCanMove(float deltaTime) const override;

	[[nodiscard]] bool Move(float deltaTime) override;
	[[nodiscard]] bool MoveLeft(float deltaTime) override;
	[[nodiscard]] bool MoveRight(float deltaTime) override;
	[[nodiscard]] bool MoveUp(float deltaTime) override;
	[[nodiscard]] bool MoveDown(float deltaTime) override;

	[[nodiscard]] ObjRectangle GetBulletPathRect(float deltaTime) const;
	[[nodiscard]] FPoint GetBulletNextPoint(float deltaTime) const;

public:
	MoveLikeBulletBeh(ObjRectangle& rect, Direction& dir, float& speed, buuid& uuid, double& damageRadius,
	                  UPoint& windowSize, std::vector<std::shared_ptr<BaseObj>>& bulletTargets,
	                  std::vector<std::shared_ptr<BaseObj>>* allObjects);

	~MoveLikeBulletBeh() override = default;

	[[nodiscard]] std::vector<Direction> GetFreePathSides(float deltaTime) const override;
};
