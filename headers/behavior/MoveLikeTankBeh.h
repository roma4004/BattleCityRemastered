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
	//NOTE: indexed by Direction
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

	[[nodiscard]] static double ShiftToClear(const ObjRectangle& rect, const ObjRectangle& blocker, Direction lateral);

	//NOTE: a wall with an opening beside it - the tank is steered in rather than left standing, but only
	//when it is already more than half inside
	[[nodiscard]] bool NudgeIntoGap(Direction dir, double step,
									const std::vector<std::shared_ptr<BaseObj>>& objects,
									const std::vector<std::shared_ptr<BaseObj>>& blockers);

public:
	MoveLikeTankBeh(ObjRectangle& rect, double& speed, Uuid& uuid, BonusEffectProperty& effects,
					const GameConfig& gameConfig);

	~MoveLikeTankBeh() override = default;

	[[nodiscard]]
	bool Move(Direction dir, double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
			  std::vector<std::shared_ptr<BaseObj>>& outCollisions) override;
	//NOTE: what stands in the way of this step - the caller decides whether any of it can be moved out
	//of the way, this only measures
	[[nodiscard]] std::vector<std::shared_ptr<BaseObj>> BlockersAhead(
			Direction dir, double step, const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	[[nodiscard]] bool ApplyMoveVelocity(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects);
	void ResetVelocity();
	[[nodiscard]] std::vector<Direction> GetFreePathSides(
			double deltaTime, std::optional<Direction> excludeDirection,
			const std::vector<std::shared_ptr<BaseObj>>& objects) const;
};
