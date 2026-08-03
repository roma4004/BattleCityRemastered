#include "behavior/MoveLikeTankBeh.h"
#include "Point.h"
#include "application/GameConfig.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "utils/ColliderUtils.h"
#include <algorithm>
#include <memory>
#include <ranges>

MoveLikeTankBeh::MoveLikeTankBeh(ObjRectangle& rect, Direction& dir, float& speed, buuid& uuid, UPoint& windowSize,
								 std::string& name, std::string& fraction,
								 std::vector<std::shared_ptr<BaseObj>>* allObjects, BonusEffectProperty& effects,
								 GameConfig& gameConfig)
	: _uuid{uuid}
	, _rect{rect}
	, _direction{dir}
	, _speed{speed}
	, _effects{effects}
	, _windowSize{windowSize}
	, _name{name}
	, _fraction{fraction}
	, _gameConfig{gameConfig}
	, _allObjects{allObjects} {}

ObjRectangle MoveLikeTankBeh::GetNextPosRect(const double deltaTime, const Direction dir) const
{
	const float speed = _speed * static_cast<float>(deltaTime);//TODO: speed from float to double, as well as rectangle
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

bool MoveLikeTankBeh::IsCanMove(const double deltaTime, const Direction dir) const
{
	const ObjRectangle tankNextPosRect = GetNextPosRect(deltaTime, dir);

	return std::ranges::none_of(*_allObjects, [uuid = _uuid, tankNextPosRect](const std::shared_ptr<BaseObj>& object)
	{
		return uuid != object->GetUuid()
			   && ColliderUtils::IsCollide(tankNextPosRect, object->GetRect())
			   && !object->GetIsPassable();
	});
}

std::vector<std::shared_ptr<BaseObj>> MoveLikeTankBeh::GetTouchedObjects(const double deltaTime) const
{
	const ObjRectangle tankNextPosRect = GetNextPosRect(deltaTime, _direction);

	auto collisions = *_allObjects | std::views::filter([this, tankNextPosRect](const std::shared_ptr<BaseObj>& object)
	{
		return _uuid != object->GetUuid()
			   && ColliderUtils::IsCollide(tankNextPosRect, object->GetRect())
			   && !object->GetIsPassable();
	});

	return std::vector<std::shared_ptr<BaseObj>>{collisions.begin(), collisions.end()};

	//C++ 23: TODO:
	// return *_allObjects | std::views::filter([this, &tankNextPosRect](const auto& obj)
	// {
	// 	return obj->GetUuid() != _uuid
	// 		&& ColliderUtils::IsCollide(tankNextPosRect, obj->GetRect())
	// 		&& !obj->GetIsPassable();
	// })
	// | std::ranges::to<std::vector>();
}

// inline float Distance(const FPoint a, const FPoint b)
// {
// 	return static_cast<float>(std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2)));
// }

float MoveLikeTankBeh::FindMinDistance(const std::vector<std::shared_ptr<BaseObj>>& objects,
									   const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const
{
	const auto [maxX, maxY] = _windowSize;
	auto minDist = static_cast<float>(maxX * maxY);
	// float nearestDist = 0;
	for (const auto& object: objects)
	{
		if (object != nullptr)
		{
			// auto getSide = [](const std::shared_ptr<BaseObj>& object) -> float { return object->GetX() + object->GetWidth();};
			const float distance = std::abs(sideDiff(object));
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
						   std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (dir == Direction::UP)
	{
		return MoveUp(deltaTime, outCollisions);
	}

	if (dir == Direction::LEFT)
	{
		return MoveLeft(deltaTime, outCollisions);
	}

	if (dir == Direction::DOWN)
	{
		return MoveDown(deltaTime, outCollisions);
	}

	if (dir == Direction::RIGHT)
	{
		return MoveRight(deltaTime, outCollisions);
	}

	return false;
}

bool MoveLikeTankBeh::MoveUp(const double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (float speed = _speed * static_cast<float>(deltaTime);
		_rect.y - speed >= 0.0f)
	{
		constexpr float maxMoveStep = 8.0f;
		speed = std::min(speed, maxMoveStep);
		if (IsCanMove(deltaTime, _direction))
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
				_rect.y -= std::floor(speed);
			}

			return true;
		}

		// move less than speed to stand next to an object
		const auto& getSideDiff = [thisTopSide = _rect.y](const std::shared_ptr<BaseObj>& object) -> float
		{
			return object->GetBottomSide() - thisTopSide;
		};

		constexpr float padding = 1.f;
		outCollisions = GetTouchedObjects(deltaTime);
		if (const float distance = FindMinDistance(outCollisions, getSideDiff) - padding; distance > 0.f)
		{
			_rect.y -= distance;

			return true;
		}
	}

	return false;
}

bool MoveLikeTankBeh::MoveLeft(const double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (float speed = _speed * static_cast<float>(deltaTime);
		_rect.x - speed >= 0.f)
	{
		constexpr float maxMoveStep = 8.0f;
		speed = std::min(speed, maxMoveStep);
		if (IsCanMove(deltaTime, _direction))
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
				_rect.x -= std::floor(speed);
			}

			return true;
		}

		// move less than speed to stand next to an object
		const auto getSideDiff = [thisLeftSide = _rect.x](const std::shared_ptr<BaseObj>& object) -> float
		{
			return thisLeftSide - object->GetRightSide();
		};

		constexpr float padding = 1.f;
		outCollisions = GetTouchedObjects(deltaTime);
		if (const float distance = FindMinDistance(outCollisions, getSideDiff) - padding; distance > 0.f)
		{
			_rect.x -= distance;

			return true;
		}
	}

	return false;
}

bool MoveLikeTankBeh::MoveDown(const double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	if (float speed = _speed * static_cast<float>(deltaTime);
		_rect.Bottom() + speed < static_cast<float>(_windowSize.y))
	{
		constexpr float maxMoveStep = 8.0f;
		speed = std::min(speed, maxMoveStep);
		if (IsCanMove(deltaTime, _direction))
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
				_rect.y += std::floor(speed);
			}

			return true;
		}

		// move less than speed to stand next to an object
		const auto getSideDiff = [thisBottomSide = _rect.Bottom()](const std::shared_ptr<BaseObj>& object) -> float
		{
			return object->GetY() - thisBottomSide;
		};

		constexpr float padding = 1.f;
		outCollisions = GetTouchedObjects(deltaTime);
		if (const float distance = FindMinDistance(outCollisions, getSideDiff) - padding; distance > 0.f)
		{
			_rect.y += distance;

			return true;
		}
	}

	return false;
}

bool MoveLikeTankBeh::MoveRight(const double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions)
{
	const float maxX = static_cast<float>(_windowSize.x - _gameConfig.sideBarWidth);
	if (float speed = _speed * static_cast<float>(deltaTime);
		_rect.Right() + speed < maxX)
	{
		constexpr float maxMoveStep = 8.0f;
		speed = std::min(speed, maxMoveStep);
		if (IsCanMove(deltaTime, _direction))
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
				_rect.x += std::floor(speed);
			}

			return true;
		}

		// move less than speed to stand next to an object
		auto getSideDiff = [thisRightSide = _rect.Right()](const std::shared_ptr<BaseObj>& object) -> float
		{
			return object->GetX() - thisRightSide;
		};

		constexpr float padding = 1.f;
		outCollisions = GetTouchedObjects(deltaTime);
		if (const float distance = FindMinDistance(outCollisions, getSideDiff) - padding; distance > 0.f)
		{
			_rect.x += distance;

			return true;
		}
	}

	return false;
}

bool MoveLikeTankBeh::ApplyMoveVelocity(const double deltaTime)
{
	bool isDrift{false};
	float speed = _speed * static_cast<float>(deltaTime);
	if (_upVelocity > speed)
	{
		if (_upVelocity > _rect.h / _driftMultiplicator)//enabling drift with delay
		{
			speed /= _driftMultiplicator;//slow down if push the gas in drift
		}

		if (IsCanMove(deltaTime, Direction::UP) && _rect.y - speed >= 0.0f)
		{
			_rect.y -= std::floor(speed);
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

		if (IsCanMove(deltaTime, Direction::LEFT) && _rect.x - speed >= 0.f)
		{
			_rect.x -= std::floor(speed);
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

		if (IsCanMove(deltaTime, Direction::DOWN) && _rect.Bottom() + speed < static_cast<float>(_windowSize.y))
		{
			_rect.y += std::floor(speed);
		}

		_downVelocity -= speed;
		isDrift = true;
	}


	const float maxX = static_cast<float>(_windowSize.x - _gameConfig.sideBarWidth);
	if (_rightVelocity > speed)
	{
		if (_rightVelocity > _rect.w / _driftMultiplicator)//enabling drift with delay
		{
			speed /= _driftMultiplicator;//slow down if push the gas in drift
		}

		if (IsCanMove(deltaTime, Direction::RIGHT) && _rect.Right() + speed < maxX)
		{
			_rect.x += std::floor(speed);
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
	_upVelocity = 0.f;
	_leftVelocity = 0.f;
	_downVelocity = 0.f;
	_rightVelocity = 0.f;
}

std::vector<Direction> MoveLikeTankBeh::GetFreePathSides(const double deltaTime,
														 const std::optional<Direction> excludeDirection) const
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

		if (IsCanMove(deltaTime, dir))
		{
			freePath.emplace_back(dir);
		}
	}

	return freePath;
}
