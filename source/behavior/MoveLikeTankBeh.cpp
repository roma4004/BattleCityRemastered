#include "behavior/MoveLikeTankBeh.h"
#include "geometry/Point.h"
#include "application/GameConfig.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "utils/ColliderUtils.h"
#include "utils/DirectionUtils.h"
#include "utils/ObjectUtils.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>

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
	if (_effects.isShipActive && dynamic_cast<const WaterTile*>(object.get()) != nullptr)
	{
		return false;
	}

	return ColliderUtils::IsCollide(nextPosRect, object->GetRect());
}

bool MoveLikeTankBeh::IsCanMove(const double deltaTime, const Direction dir,
								const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	const ObjRectangle tankNextPosRect = DirectionUtils::Sweep(_rect, _speed * deltaTime, dir);

	auto blocking = [this, &tankNextPosRect](const std::shared_ptr<BaseObj>& object)
	{
		return IsBlocking(object, tankNextPosRect);
	};

	return std::ranges::none_of(objects, blocking);
}

//NOTE: distance and contacts in one pass - the same list used to be walked three times
double MoveLikeTankBeh::GetTravelledDistance(const double step, const Direction dir,
											 const std::vector<std::shared_ptr<BaseObj>>& objects,
											 std::vector<std::shared_ptr<BaseObj>>& outTouched) const
{
	const ObjRectangle sweptRect = DirectionUtils::Sweep(_rect, step, dir);

	//NOTE: park a pixel short - IsCollide reads a flush touch as a collision
	constexpr double padding = 1.0;
	double travelled = step;
	outTouched.clear();
	for (const std::shared_ptr<BaseObj>& object: objects)
	{
		if (!IsBlocking(object, sweptRect))
		{
			continue;
		}

		outTouched.push_back(object);
		//NOTE: a negative gap is level with or behind the leading edge - touched, but not in the way
		if (const double gap = DirectionUtils::GapTo(_rect, object->GetRect(), dir); gap >= 0.0)
		{
			travelled = std::min(travelled, gap - padding);
		}
	}

	return travelled;
}

bool MoveLikeTankBeh::Move(const Direction dir, const double deltaTime,
						   const std::vector<std::shared_ptr<BaseObj>>& objects,
						   std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	const double step = _speed * deltaTime;
	if (!DirectionUtils::FitsBeforeEdge(_rect, _gameConfig.battlefieldSize, step, dir))
	{
		return false;
	}

	const double distance = GetTravelledDistance(step, dir, objects, outCollisions);

	//NOTE: ice turns the step into momentum, but only while the way is clear
	if (outCollisions.empty() && _effects.isTouchTheIce)
	{
		if (double& velocity = _velocity[static_cast<size_t>(dir)];
			velocity < DirectionUtils::SideAlong(_rect, dir) * _driftMultiplicator)// clamp max accumulated velocity
		{
			velocity += distance * _driftMultiplicator;
		}

		return true;
	}

	if (distance <= 0.0)
	{
		return false;
	}

	_rect = DirectionUtils::Advance(_rect, distance, dir);

	return true;
}

bool MoveLikeTankBeh::ApplyMoveVelocity(const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects)
{
	bool isDrift{false};
	double speed = _speed * deltaTime;
	for (const Direction dir: {Direction::UP, Direction::LEFT, Direction::DOWN, Direction::RIGHT})
	{
		double& velocity = _velocity[static_cast<size_t>(dir)];
		if (velocity <= speed)
		{
			continue;
		}

		if (velocity > DirectionUtils::SideAlong(_rect, dir) / _driftMultiplicator)//enabling drift with delay
		{
			speed /= _driftMultiplicator;//slow down if push the gas in drift
		}

		if (IsCanMove(deltaTime, dir, objects)
			&& DirectionUtils::FitsBeforeEdge(_rect, _gameConfig.battlefieldSize, speed, dir))
		{
			_rect = DirectionUtils::Advance(_rect, speed, dir);
		}

		velocity -= speed;
		isDrift = true;
	}

	return isDrift;
}

void MoveLikeTankBeh::ResetVelocity() { _velocity.fill(0.0); }

std::vector<Direction> MoveLikeTankBeh::GetFreePathSides(const double deltaTime,
														 const std::optional<Direction> excludeDirection,
														 const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	std::vector<Direction> freePath;

	constexpr int defaultCollisionReserve{4};
	freePath.reserve(defaultCollisionReserve);

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
