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
	std::optional<Direction> _carriedTo{};
	const GameConfig& _gameConfig;

	[[nodiscard]] bool IsBlocking(const BaseObj* object, const ObjRectangle& nextPosRect) const;
	[[nodiscard]] bool IsCanMove(double deltaTime, Direction dir,
								 const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	//NOTE: how far the tank actually gets - the frame step is the ceiling, not the answer
	[[nodiscard]] double GetTravelledDistance(double step, Direction dir,
											  const std::vector<std::shared_ptr<BaseObj>>& objects,
											  std::vector<BaseObj*>& outTouched) const;

	//NOTE: steers the tank into an opening beside a wall, only when more than half of it is already in line
	[[nodiscard]] bool NudgeIntoGap(Direction dir, double step,
									const std::vector<std::shared_ptr<BaseObj>>& objects,
									const std::vector<BaseObj*>& blockers);

	[[nodiscard]] std::vector<double> GrowthShifts(bool isXAxis, double side,
												   const std::vector<const BaseObj*>& near) const;

public:
	MoveLikeTankBeh(ObjRectangle& rect, double& speed, Uuid& uuid, BonusEffectProperty& effects,
					const GameConfig& gameConfig);

	~MoveLikeTankBeh() override = default;

	[[nodiscard]]
	bool Move(Direction dir, double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
			  std::vector<BaseObj*>& outCollisions) override;
	[[nodiscard]] std::vector<BaseObj*> BlockersAhead(
			Direction dir, double step, const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	//NOTE: how far the tank gets before the blocker - no limit from one it is already past
	[[nodiscard]] double SpaceBefore(const BaseObj& blocker, Direction dir) const;
	[[nodiscard]] bool ApplyMoveVelocity(double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects);
	[[nodiscard]] double GetVelocity(Direction dir) const;
	void Carry(Direction dir, double distance, double velocity);
	void ResetVelocity();
	[[nodiscard]] std::vector<Direction> GetFreePathSides(
			double deltaTime, std::optional<Direction> excludeDirection,
			const std::vector<std::shared_ptr<BaseObj>>& objects) const;
	//NOTE: grows where nothing stands, shifted off a wall beside it; with no room the hull stays as it was
	[[nodiscard]] bool TryGrow(double side, const std::vector<std::shared_ptr<BaseObj>>& objects);
};
