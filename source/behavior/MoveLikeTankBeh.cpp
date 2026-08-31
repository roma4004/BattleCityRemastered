#include "behavior/MoveLikeTankBeh.h"
#include "geometry/Point.h"
#include "application/GameConfig.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "utils/ColliderUtils.h"
#include "utils/ObjectUtils.h"
#include <algorithm>
#include <cmath>
#include <memory>
#include <ranges>

MoveLikeTankBeh::MoveLikeTankBeh(ObjRectangle& rect, Direction& dir, double& speed, Uuid& uuid,
								 BonusEffectProperty& effects, const GameConfig& gameConfig)
	: _uuid{uuid}
	, _rect{rect}
	, _direction{dir}
	, _speed{speed}
	, _effects{effects}
	, _gameConfig{gameConfig} {}

ObjRectangle MoveLikeTankBeh::GetNextPosRect(const double deltaTime, const Direction dir) const
{
	const double speed = _speed * deltaTime;
	const auto [x, y, w, h] = _rect;
	if (dir == Direction::UP)
	{
		return ObjRectangle{.x = x, .y = y - speed, .w = w, .h = h + speed};
	}

	if (dir == Direction::LEFT)
	{
		return ObjRectangle{.x = x - speed, .y = y, .w = w + speed, .h = h};
	}

	if (dir == Direction::DOWN)
	{
		return ObjRectangle{.x = x, .y = y, .w = w, .h = h + speed};
	}

	if (dir == Direction::RIGHT)
	{
		return ObjRectangle{.x = x, .y = y, .w = w + speed, .h = h};
	}

	return ObjRectangle{};
}

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
	const ObjRectangle tankNextPosRect = GetNextPosRect(deltaTime, dir);

	auto blocking = [this, &tankNextPosRect](const std::shared_ptr<BaseObj>& object)
	{
		return IsBlocking(object, tankNextPosRect);
	};

	return std::ranges::none_of(objects, blocking);
}

std::vector<std::shared_ptr<BaseObj>> MoveLikeTankBeh::GetTouchedObjects(
		const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects) const
{
	auto blocking = [this, tankNextPosRect = GetNextPosRect(deltaTime, _direction)](const auto& obj)
	{
		return IsBlocking(obj, tankNextPosRect);
	};

	return objects
		   | std::views::filter(blocking)
		   | std::ranges::to<std::vector>();
}

// inline float Distance(const FPoint a, const FPoint b)
// {
// 	return static_cast<float>(std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2)));
// }

double MoveLikeTankBeh::FindMinDistance(const std::vector<std::shared_ptr<BaseObj>>& objects,
										const std::function<double(const std::shared_ptr<BaseObj>&)>& sideDiff) const
{
	const auto [maxX, maxY] = _gameConfig.battlefieldSize;
	auto minDist = static_cast<double>(maxX * maxY);
	// float nearestDist = 0.f;
	for (const auto& object: objects)
	{
		if (object != nullptr)
		{
			// auto getSide = [](const std::shared_ptr<BaseObj>& object) -> float { return object->GetX() + object->GetWidth();};
			const double distance = std::abs(sideDiff(object));
			// const float distance = abs(this->GetX() - object->GetX() + object->GetWidth());
			if (distance < minDist)//TODO: need minimal abs distance
			{
				minDist = distance;
			}
		}
	}

	return minDist;

	// constexpr auto padding = 1.f;
	// float distance = this->GetX() - nearestX - padding;
	// if (distance < padding)
	// {
	// 	return 0.f;
	// }
	//
	// return distance;
}

bool MoveLikeTankBeh::Move(const Direction dir, const double deltaTime,
						   const std::vector<std::shared_ptr<BaseObj>>& objects,
						   std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (dir == Direction::UP)
	{
		return MoveUp(deltaTime, objects, outCollisions);
	}

	if (dir == Direction::LEFT)
	{
		return MoveLeft(deltaTime, objects, outCollisions);
	}

	if (dir == Direction::DOWN)
	{
		return MoveDown(deltaTime, objects, outCollisions);
	}

	if (dir == Direction::RIGHT)
	{
		return MoveRight(deltaTime, objects, outCollisions);
	}

	return false;
}

bool MoveLikeTankBeh::MoveUp(const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
							 std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (double speed = _speed * deltaTime;
		_rect.y - speed >= 0.0)
	{
		constexpr double maxMoveStep = 8.0;
		speed = std::min(speed, maxMoveStep);
		if (IsCanMove(deltaTime, _direction, objects))
		{
			if (_effects.isTouchTheIce)
			{
				if (_upVelocity < _rect.h * _driftMultiplicator)// clamp max accumulated velocity
				{
					_upVelocity += speed * _driftMultiplicator;
				}
			}
			else
			{
				_rect.y -= speed;
			}

			return true;
		}

		// move less than speed to stand next to an object
		const auto& getSideDiff = [thisTopSide = _rect.y](const std::shared_ptr<BaseObj>& object) -> double
		{
			return object->GetBottomSide() - thisTopSide;
		};

		constexpr double padding = 1.0;
		outCollisions = GetTouchedObjects(deltaTime, objects);
		//NOTE: never further than this frame's step - FindMinDistance seeds on the field area, so an
		//empty list, or a tank already inside an obstacle, would otherwise teleport it across the map
		if (const double distance = std::min(FindMinDistance(outCollisions, getSideDiff) - padding, speed);
			distance > 0.0)
		{
			_rect.y -= distance;

			return true;
		}
	}

	return false;
}

bool MoveLikeTankBeh::MoveLeft(const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
							   std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (double speed = _speed * deltaTime;
		_rect.x - speed >= 0.0)
	{
		constexpr double maxMoveStep = 8.0;
		speed = std::min(speed, maxMoveStep);
		if (IsCanMove(deltaTime, _direction, objects))
		{
			if (_effects.isTouchTheIce)
			{
				if (_leftVelocity < _rect.w * _driftMultiplicator)// clamp max accumulated velocity
				{
					_leftVelocity += speed * _driftMultiplicator;
				}
			}
			else
			{
				_rect.x -= speed;
			}

			return true;
		}

		// move less than speed to stand next to an object
		const auto getSideDiff = [thisLeftSide = _rect.x](const std::shared_ptr<BaseObj>& object) -> double
		{
			return thisLeftSide - object->GetRightSide();
		};

		constexpr double padding = 1.0;
		outCollisions = GetTouchedObjects(deltaTime, objects);
		//NOTE: never further than this frame's step - FindMinDistance seeds on the field area, so an
		//empty list, or a tank already inside an obstacle, would otherwise teleport it across the map
		if (const double distance = std::min(FindMinDistance(outCollisions, getSideDiff) - padding, speed);
			distance > 0.0)
		{
			_rect.x -= distance;

			return true;
		}
	}

	return false;
}

bool MoveLikeTankBeh::MoveDown(const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
							   std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (double speed = _speed * deltaTime;
		_rect.Bottom() + speed < static_cast<double>(_gameConfig.battlefieldSize.y))
	{
		constexpr double maxMoveStep = 8.0;
		speed = std::min(speed, maxMoveStep);
		if (IsCanMove(deltaTime, _direction, objects))
		{
			if (_effects.isTouchTheIce)
			{
				if (_downVelocity < _rect.h * _driftMultiplicator)// clamp max accumulated velocity
				{
					_downVelocity += speed * _driftMultiplicator;
				}
			}
			else
			{
				_rect.y += speed;
			}

			return true;
		}

		// move less than speed to stand next to an object
		const auto getSideDiff = [thisBottomSide = _rect.Bottom()](const std::shared_ptr<BaseObj>& object) -> double
		{
			return object->GetY() - thisBottomSide;
		};

		constexpr double padding = 1.0;
		outCollisions = GetTouchedObjects(deltaTime, objects);
		//NOTE: never further than this frame's step - FindMinDistance seeds on the field area, so an
		//empty list, or a tank already inside an obstacle, would otherwise teleport it across the map
		if (const double distance = std::min(FindMinDistance(outCollisions, getSideDiff) - padding, speed);
			distance > 0.0)
		{
			_rect.y += distance;

			return true;
		}
	}

	return false;
}

bool MoveLikeTankBeh::MoveRight(const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects,
								std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	const double maxX = static_cast<double>(_gameConfig.battlefieldSize.x);
	if (double speed = _speed * deltaTime;
		_rect.Right() + speed < maxX)
	{
		constexpr double maxMoveStep = 8.0;
		speed = std::min(speed, maxMoveStep);
		if (IsCanMove(deltaTime, _direction, objects))
		{
			if (_effects.isTouchTheIce)
			{
				if (_rightVelocity < _rect.w * _driftMultiplicator)// clamp max accumulated velocity
				{
					_rightVelocity += speed * _driftMultiplicator;
				}
			}
			else
			{
				_rect.x += speed;
			}

			return true;
		}

		// move less than speed to stand next to an object
		auto getSideDiff = [thisRightSide = _rect.Right()](const std::shared_ptr<BaseObj>& object) -> double
		{
			return object->GetX() - thisRightSide;
		};

		constexpr double padding = 1.0;
		outCollisions = GetTouchedObjects(deltaTime, objects);
		//NOTE: never further than this frame's step - FindMinDistance seeds on the field area, so an
		//empty list, or a tank already inside an obstacle, would otherwise teleport it across the map
		if (const double distance = std::min(FindMinDistance(outCollisions, getSideDiff) - padding, speed);
			distance > 0.0)
		{
			_rect.x += distance;

			return true;
		}
	}

	return false;
}

bool MoveLikeTankBeh::ApplyMoveVelocity(const double deltaTime, const std::vector<std::shared_ptr<BaseObj>>& objects)
{
	bool isDrift{false};
	double speed = _speed * deltaTime;
	if (_upVelocity > speed)
	{
		if (_upVelocity > _rect.h / _driftMultiplicator)//enabling drift with delay
		{
			speed /= _driftMultiplicator;//slow down if push the gas in drift
		}

		if (IsCanMove(deltaTime, Direction::UP, objects) && _rect.y - speed >= 0.0)
		{
			_rect.y -= speed;
		}

		_upVelocity -= speed;
		isDrift = true;
	}

	if (_leftVelocity > speed)
	{
		if (_leftVelocity > _rect.w / _driftMultiplicator)//enabling drift with delay
		{
			speed /= _driftMultiplicator;//slow down if push the gas in drift
		}

		if (IsCanMove(deltaTime, Direction::LEFT, objects) && _rect.x - speed >= 0.0)
		{
			_rect.x -= speed;
		}

		_leftVelocity -= speed;
		isDrift = true;
	}

	if (_downVelocity > speed)
	{
		if (_downVelocity > _rect.h / _driftMultiplicator)//enabling drift with delay
		{
			speed /= _driftMultiplicator;//slow down if push the gas in drift
		}

		const double maxY = static_cast<double>(_gameConfig.battlefieldSize.y);
		if (IsCanMove(deltaTime, Direction::DOWN, objects) && _rect.Bottom() + speed < maxY)
		{
			_rect.y += speed;
		}

		_downVelocity -= speed;
		isDrift = true;
	}

	const double maxX = static_cast<double>(_gameConfig.battlefieldSize.x);
	if (_rightVelocity > speed)
	{
		if (_rightVelocity > _rect.w / _driftMultiplicator)//enabling drift with delay
		{
			speed /= _driftMultiplicator;//slow down if push the gas in drift
		}

		if (IsCanMove(deltaTime, Direction::RIGHT, objects) && _rect.Right() + speed < maxX)
		{
			_rect.x += speed;
		}

		_rightVelocity -= speed;
		isDrift = true;
	}

	if (isDrift)
	{
		return true;
	}

	return false;
}

void MoveLikeTankBeh::ResetVelocity()
{
	_upVelocity = 0.0;
	_leftVelocity = 0.0;
	_downVelocity = 0.0;
	_rightVelocity = 0.0;
}

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
