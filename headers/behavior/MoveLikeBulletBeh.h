#pragma once

#include "entities/BulletCalibre.h"
#include "interfaces/IMoveBeh.h"
#include <boost/uuid/uuid.hpp>
#include <memory>

// enum class Direction : char8_t;
class Bullet;
struct FPoint;
struct ObjRectangle;
class EventSystem;
class GameConfig;

class MoveLikeBulletBeh final : public IMoveBeh
{
	using buuid = boost::uuids::uuid;

	buuid& _uuid;
	ObjRectangle& _rect;
	Direction& _dir;
	GameConfig& _gameConfig;
	BulletCalibre _calibre{};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>> GetCircleCollisionObjects(FPoint blowCenter) const;
	[[nodiscard]] bool IsCanMove(double deltaTime) const override;
	[[nodiscard]] ObjRectangle GetNextPos(double deltaTime) const;
	[[nodiscard]] FPoint GetBulletNextPoint(double deltaTime) const;

protected:
	[[nodiscard]] bool MoveLeft(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveRight(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveUp(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveDown(double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;

public:
	MoveLikeBulletBeh(ObjRectangle& rect, Direction& dir, buuid& uuid, GameConfig& gameConfig,
					  const BulletCalibre& calibre, std::vector<std::shared_ptr<BaseObj>>* allObjects);

	~MoveLikeBulletBeh() override = default;

	[[nodiscard]]
	bool Move(Direction dir, double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
};
