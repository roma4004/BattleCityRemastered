#include "behavior/MoveLikeTankBeh.h"
#include "geometry/Point.h"
#include "application/GameConfig.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/Terrain.h"
#include "utils/ColliderUtils.h"
#include "utils/DirectionUtils.h"
#include "utils/ObjectUtils.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <tuple>
#include <vector>

MoveLikeTankBeh::MoveLikeTankBeh(ObjRectangle& rect, double& speed, Uuid& uuid, BonusEffectProperty& effects,
								 const GameConfig& gameConfig)
	: _uuid{uuid}
	, _rect{rect}
	, _speed{speed}
	, _effects{effects}
	, _gameConfig{gameConfig} {}

bool MoveLikeTankBeh::IsBlocking(const std::shared_ptr<BaseObj>& object, const ObjRectangle& nextPosRect) const
{
	if (!ObjectUtils::IsAlive(object) || _uuid == object->GetUuid() || object->GetIsPassable())
	{
		return false;
	}

	//NOTE: the ship bonus carries the tank over the water for the rest of its life
	if (_effects.isShipActive && object->GetTerrain() == Terrain::Water)
	{
		return false;
	}

	return ColliderUtils::IsCollide(nextPosRect, object->GetRect());
}

bool MoveLikeTankBeh::IsCanMove(const double deltaTime, const Direction dir,
								const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	const ObjRectangle tankNextPosRect{DirectionUtils::Swept(_rect, _speed * deltaTime, dir)};

	auto blocking = [this, &tankNextPosRect](const std::shared_ptr<BaseObj>& object)
	{
		return IsBlocking(object, tankNextPosRect);
	};

	return std::ranges::none_of(objects, blocking);
}

double MoveLikeTankBeh::GetTravelledDistance(const double step, const Direction dir,
											 const std::vector<std::shared_ptr<BaseObj>>& objects,
											 std::vector<std::shared_ptr<BaseObj>>& outTouched) const
{
	const ObjRectangle sweptRect{DirectionUtils::Swept(_rect, step, dir)};

	//NOTE: park a pixel short - flush is the knife edge where rounding alone decides which side of the
	//wall the leading edge is on
	constexpr double padding{1.0};
	double travelled{step};
	outTouched.clear();
	for (const std::shared_ptr<BaseObj>& object: objects)
	{
		if (!IsBlocking(object, sweptRect))
		{
			continue;
		}

		outTouched.push_back(object);
		//NOTE: only what is behind the leading edge is out of the way - a tank standing against a wall is
		//a hair inside it as often as a hair short of it, and a bare sign test lets that hair through
		if (const double gap{DirectionUtils::GapTo(_rect, object->GetRect(), dir)};
			gap > -ColliderUtils::kTouchTolerance)
		{
			travelled = std::min(travelled, gap - padding);
		}
	}

	return travelled;
}

//NOTE: exactly onto the edge, not a pixel past - a corridor is cut to the tank's own width, so any
//overshoot lands in the far wall
double MoveLikeTankBeh::ShiftToClear(const ObjRectangle& rect, const ObjRectangle& blocker, const Direction lateral)
{
	switch (lateral)
	{
		case Direction::LEFT:
			return rect.Right() - blocker.x;
		case Direction::RIGHT:
			return blocker.Right() - rect.x;
		case Direction::UP:
			return rect.Bottom() - blocker.y;
		case Direction::DOWN:
			return blocker.Bottom() - rect.y;
	}

	return 0.0;
}

bool MoveLikeTankBeh::NudgeIntoGap(const Direction dir, const double step,
								   const std::vector<std::shared_ptr<BaseObj>>& objects,
								   const std::vector<std::shared_ptr<BaseObj>>& blockers)
{
	//NOTE: half the tank across - less than that inside the opening is not aiming for it, it is missing it
	const double reach{DirectionUtils::SizeAlong(_rect, DirectionUtils::Laterals(dir).front()) / 2.0};

	for (const Direction lateral: DirectionUtils::Laterals(dir))
	{
		double needed{};
		for (const std::shared_ptr<BaseObj>& blocker: blockers)
		{
			needed = std::max(needed, ShiftToClear(_rect, blocker->GetRect(), lateral));
		}

		//NOTE: the border is not an opening - without this the nudge walks the tank off the field, and
		//nothing outside it ever blocks the way back
		if (needed <= 0.0 || needed > reach
			|| !DirectionUtils::FitsBeforeEdge(_rect, _gameConfig.battlefieldSize, needed, lateral))
		{
			continue;
		}

		//NOTE: the opening has to take the tank whole - clearing this wall into the next one is no help
		const ObjRectangle aligned{DirectionUtils::Moved(_rect, needed, lateral)};
		auto blocking = [this, &aligned, step, dir](const std::shared_ptr<BaseObj>& object)
		{
			return IsBlocking(object, DirectionUtils::Swept(aligned, step, dir));
		};

		if (std::ranges::any_of(objects, blocking))
		{
			continue;
		}

		//NOTE: the same ceiling the forward step has, and over the same checked path
		std::vector<std::shared_ptr<BaseObj>> touched;
		const double distance{GetTravelledDistance(std::min(step, needed), lateral, objects, touched)};
		if (distance <= 0.0)
		{
			continue;
		}

		_rect = DirectionUtils::Moved(_rect, distance, lateral);

		return true;
	}

	return false;
}

std::vector<std::shared_ptr<BaseObj>> MoveLikeTankBeh::BlockersAhead(
		const Direction dir, const double step, const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	std::vector<std::shared_ptr<BaseObj>> blockers{};
	std::ignore = GetTravelledDistance(step, dir, objects, blockers);

	return blockers;
}

bool MoveLikeTankBeh::Move(const Direction dir, const double deltaTime,
						   const std::vector<std::shared_ptr<BaseObj>>& objects,
						   std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	const double step{_speed * deltaTime};
	if (!DirectionUtils::FitsBeforeEdge(_rect, _gameConfig.battlefieldSize, step, dir))
	{
		return false;
	}

	const double distance{GetTravelledDistance(step, dir, objects, outCollisions)};

	//NOTE: ice turns the step into momentum, but only while the way is clear
	if (outCollisions.empty() && _effects.isTouchTheIce)
	{
		const double maxVelocity{DirectionUtils::SizeAlong(_rect, dir) * _driftMultiplicator};
		if (double& velocity{_velocity[static_cast<size_t>(dir)]}; velocity < maxVelocity)
		{
			velocity += distance * _driftMultiplicator;
		}

		return true;
	}

	if (distance <= 0.0)
	{
		return NudgeIntoGap(dir, step, objects, outCollisions);
	}

	_rect = DirectionUtils::Moved(_rect, distance, dir);

	return true;
}

bool MoveLikeTankBeh::ApplyMoveVelocity(const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects)
{
	bool isDrift{};
	double speed{_speed * deltaTime};
	for (const Direction dir: {Direction::UP, Direction::LEFT, Direction::DOWN, Direction::RIGHT})
	{
		double& velocity{_velocity[static_cast<size_t>(dir)]};
		if (velocity <= speed)
		{
			continue;
		}

		//NOTE: past this much momentum the tank is sliding, and driving into the slide is slower than steering out
		if (velocity > DirectionUtils::SizeAlong(_rect, dir) / _driftMultiplicator)
		{
			speed /= _driftMultiplicator;
		}

		if (IsCanMove(deltaTime, dir, objects)
			&& DirectionUtils::FitsBeforeEdge(_rect, _gameConfig.battlefieldSize, speed, dir))
		{
			_rect = DirectionUtils::Moved(_rect, speed, dir);
		}

		velocity -= speed;
		isDrift = true;
	}

	return isDrift;
}

void MoveLikeTankBeh::ResetVelocity() { _velocity.fill(0.0); }

std::vector<Direction> MoveLikeTankBeh::GetFreePathSides(
		const double deltaTime, const std::optional<Direction> excludeDirection,
		const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	std::vector<Direction> freePath;
	freePath.reserve(4u);

	for (const Direction dir: {Direction::UP, Direction::LEFT, Direction::DOWN, Direction::RIGHT})
	{
		if (excludeDirection == dir)
		{
			continue;
		}

		if (IsCanMove(deltaTime, dir, objects))
		{
			freePath.emplace_back(dir);
		}
	}

	return freePath;
}
