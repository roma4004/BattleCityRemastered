#pragma once

#include "entities/BulletCalibre.h"
#include "interfaces/IMoveBeh.h"
#include "utils/Uuid.h"
#include <memory>
#include <optional>
#include <vector>

// enum class Direction : char8_t;
class Bullet;
struct FPoint;
struct ObjRectangle;
class EventSystem;
class GameConfig;

class MoveLikeBulletBeh final : public IMoveBeh
{
	Uuid& _uuid;
	const Uuid& _authorUuid;
	ObjRectangle& _rect;
	Direction& _direction;
	const GameConfig& _gameConfig;
	BulletCalibre _calibre{};

	[[nodiscard]] bool IsSelfOrAuthor(const BaseObj& object) const;
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>> GetCircleCollisionObjects(
			FPoint blowCenter, const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	[[nodiscard]] bool IsCanMove(double deltaTime, Direction dir,
								 const std::vector<std::shared_ptr<BaseObj>>& objects) const override;
	[[nodiscard]] ObjRectangle GetNextPos(double deltaTime) const;
	[[nodiscard]] FPoint GetBlowCenter(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	//NOTE: how far the bullet actually got before touching - the frame step is the ceiling, not the answer
	[[nodiscard]] double GetTravelledDistance(double deltaTime,
											  const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	[[nodiscard]] double GetGapTo(const ObjRectangle& target) const;
	[[nodiscard]] double GetGapToBattlefieldEdge() const;

protected:
	[[nodiscard]] bool MoveUp(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
							  std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveLeft(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
								std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveDown(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
								std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool MoveRight(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
								 std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;

public:
	MoveLikeBulletBeh(ObjRectangle& rect, Direction& dir, Uuid& uuid, const Uuid& authorUuid,
					  const GameConfig& gameConfig, const BulletCalibre& calibre);

	~MoveLikeBulletBeh() override = default;

	[[nodiscard]]
	bool Move(Direction dir, double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
			  std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	void Reset(const BulletCalibre& calibre);
	[[nodiscard]] std::vector<Direction> GetFreePathSides(
			double deltaTime, std::optional<Direction> excludeDirection,
			const std::vector<std::shared_ptr<BaseObj>>& objects) const override;
};
