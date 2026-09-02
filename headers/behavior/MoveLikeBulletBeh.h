#pragma once

#include "entities/BulletCalibre.h"
#include "interfaces/IMoveBeh.h"
#include "utils/Uuid.h"
#include <memory>
#include <optional>
#include <vector>

struct FPoint;
struct ObjRectangle;
class GameConfig;

class MoveLikeBulletBeh final : public IMoveBeh
{
	Uuid& _uuid;
	const Uuid& _authorUuid;
	ObjRectangle& _rect;
	const GameConfig& _gameConfig;
	const BulletCalibre& _calibre;

	[[nodiscard]] bool IsSelfOrAuthor(const BaseObj& object) const;
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>> GetCircleCollisionObjects(
			FPoint blowCenter, const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	[[nodiscard]] bool IsCanMove(double deltaTime, Direction dir,
								 const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	[[nodiscard]] FPoint GetBlowCenter(double deltaTime, Direction dir,
									   const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	//NOTE: how far the bullet actually got before touching - the frame step is the ceiling, not the answer
	[[nodiscard]] double GetTravelledDistance(double deltaTime, Direction dir,
											  const std::vector<std::shared_ptr<BaseObj>>& objects) const;

public:
	MoveLikeBulletBeh(ObjRectangle& rect, Uuid& uuid, const Uuid& authorUuid, const GameConfig& gameConfig,
					  const BulletCalibre& calibre);

	~MoveLikeBulletBeh() override = default;

	[[nodiscard]]
	bool Move(Direction dir, double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
			  std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] std::vector<Direction> GetFreePathSides(
			double deltaTime, std::optional<Direction> excludeDirection,
			const std::vector<std::shared_ptr<BaseObj>>& objects) const override;
};
