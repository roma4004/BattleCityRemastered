#pragma once

#include "interfaces/IMoveBeh.h"
#include <functional>
#include <memory>

enum Direction : char8_t;
class Bullet;
struct FPoint;
struct ObjRectangle;
class EventSystem;

class MoveLikeBulletBeh final : public IMoveBeh
{
	BaseObj* _selfParent{nullptr};//TODO: replace with weak_ptr
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;
	std::shared_ptr<EventSystem> _events{nullptr};

	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>> GetCircleCollisionObjects(FPoint blowCenter) const;
	void DealDamage(const std::vector<std::shared_ptr<BaseObj>>& objectList) const;
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>> IsCanMove(float deltaTime) const override;

	[[nodiscard]] bool Move(float deltaTime) const override;
	[[nodiscard]] bool MoveLeft(float deltaTime) const override;
	[[nodiscard]] bool MoveRight(float deltaTime) const override;
	[[nodiscard]] bool MoveUp(float deltaTime) const override;
	[[nodiscard]] bool MoveDown(float deltaTime) const override;

	[[nodiscard]] static ObjRectangle GetBulletPathRect(const Bullet* bullet, float deltaTime);
	[[nodiscard]] static FPoint GetBulletNextPoint(const Bullet* bullet, float deltaTime);

public:
	MoveLikeBulletBeh(BaseObj* parent, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	                  std::shared_ptr<EventSystem> events);

	~MoveLikeBulletBeh() override = default;

	[[nodiscard]] std::vector<Direction> GetFreePathSides(float deltaTime) const override;
};
