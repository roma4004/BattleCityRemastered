#pragma once

#include "geometry/ObjRectangle.h"
#include "interfaces/IMoveBeh.h"
#include "utils/Uuid.h"
#include <array>
#include <memory>
#include <optional>
#include <vector>

struct BonusEffectProperty;
class BaseObj;
class GameConfig;

class MoveLikeTankBeh final : public IMoveBeh
{
	Uuid& _uuid;
	ObjRectangle& _rect;
	double& _speed;
	BonusEffectProperty& _effects;
	//NOTE: indexed by Direction - four named velocities were the same four lines four times
	std::array<double, 4> _velocity{};
	double _driftMultiplicator{1.5};
	const GameConfig& _gameConfig;

	[[nodiscard]] bool IsBlocking(const std::shared_ptr<BaseObj>& object, const ObjRectangle& nextPosRect) const;
	[[nodiscard]] bool IsCanMove(double deltaTime, Direction dir,
								 const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	//NOTE: how far the tank actually gets - the frame step is the ceiling, not the answer
	[[nodiscard]] double GetTravelledDistance(double step, Direction dir,
											  const std::vector<std::shared_ptr<BaseObj>>& objects,
											  std::vector<std::shared_ptr<BaseObj>>& outTouched) const;

public:
	MoveLikeTankBeh(ObjRectangle& rect, double& speed, Uuid& uuid, BonusEffectProperty& effects,
					const GameConfig& gameConfig);

	~MoveLikeTankBeh() override = default;

	[[nodiscard]]
	bool Move(Direction dir, double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
			  std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	[[nodiscard]] bool ApplyMoveVelocity(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects);
	void ResetVelocity();
	[[nodiscard]] std::vector<Direction> GetFreePathSides(
			double deltaTime, std::optional<Direction> excludeDirection,
			const std::vector<std::shared_ptr<BaseObj>>& objects) const;
};
