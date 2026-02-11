#include "behavior/MoveLikeTankBeh.h"
#include "Point.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "interfaces/IPickupableBonus.h"
#include "utils/ColliderUtils.h"
#include <functional>
#include <memory>

///TODO: change selfParent to Tank to avoid check on each IsCanMove
MoveLikeTankBeh::MoveLikeTankBeh(ObjRectangle& rect, Direction& dir, float& speed, buuid& uuid, UPoint& windowSize,
                                 std::string& name, std::string& fraction,
                                 std::vector<std::shared_ptr<BaseObj>>& touchedObstacles,
                                 std::vector<std::shared_ptr<BaseObj>>* allObjects)
	: _uuid{uuid},
	  _rect{rect},
	  _direction{dir},
	  _speed{speed},
	  _windowSize{windowSize},
	  _name{name},
	  _fraction{fraction},
	  _touchedObstacles{touchedObstacles},
	  _allObjects{allObjects} {}

bool MoveLikeTankBeh::IsCanMove(const double deltaTime) const
{
	constexpr int defaultCollisionReserve{5};
	_touchedObstacles.reserve(defaultCollisionReserve);

	const float speed = _speed * static_cast<float>(deltaTime);
	const auto [x, y, w, h] = _rect;
	ObjRectangle tankNextPosRect;
	if (_direction == Direction::UP)
	{
		tankNextPosRect = {.x = x, .y = y - speed, .w = w, .h = h + speed};
	}
	else if (_direction == Direction::DOWN)
	{
		tankNextPosRect = {.x = x, .y = y, .w = w, .h = h + speed};
	}
	else if (_direction == Direction::LEFT)
	{
		tankNextPosRect = {.x = x - speed, .y = y, .w = w + speed, .h = h};
	}
	else if (_direction == Direction::RIGHT)
	{
		tankNextPosRect = {.x = x, .y = y, .w = w + speed, .h = h};
	}

	for (std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (_uuid == object->GetUuid())
		{
			continue;
		}

		if (ColliderUtils::IsCollide(tankNextPosRect, object->GetRect()))
		{
			if (!object->GetIsPassable())
			{
				_touchedObstacles.emplace_back(object);
			}
		}
	}

	return _touchedObstacles.empty();
}

std::vector<Direction> MoveLikeTankBeh::GetFreePathSides(const double deltaTime) const
{
	std::vector<Direction> freePath;

	constexpr int defaultCollisionReserve{4};
	freePath.reserve(defaultCollisionReserve);

	const float speed = _speed;
	const float moveSpeed = speed * static_cast<float>(deltaTime);
	const auto [x, y, w, h] = _rect;
	const ObjRectangle tankNextPosRectUp{.x = x, .y = y - moveSpeed, .w = w, .h = h + moveSpeed};
	const ObjRectangle tankNextPosRectDown{.x = x, .y = y, .w = w, .h = h + moveSpeed};
	const ObjRectangle tankNextPosRectLeft{.x = x - moveSpeed, .y = y, .w = w + moveSpeed, .h = h};
	const ObjRectangle tankNextPosRectRight{.x = x, .y = y, .w = w + moveSpeed, .h = h};

	bool isFreeUp{true};
	bool isFreeDown{true};
	bool isFreeLeft{true};
	bool isFreeRight{true};

	for (std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (_uuid == object->GetUuid())
		{
			continue;
		}

		if (isFreeUp && ColliderUtils::IsCollide(tankNextPosRectUp, object->GetRect()))
		{
			if (!object->GetIsPassable()) { isFreeUp = false; }
		}

		if (isFreeDown && ColliderUtils::IsCollide(tankNextPosRectDown, object->GetRect()))
		{
			if (!object->GetIsPassable()) { isFreeDown = false; }
		}

		if (isFreeLeft && ColliderUtils::IsCollide(tankNextPosRectLeft, object->GetRect()))
		{
			if (!object->GetIsPassable()) { isFreeLeft = false; }
		}

		if (isFreeRight && ColliderUtils::IsCollide(tankNextPosRectRight, object->GetRect()))
		{
			if (!object->GetIsPassable()) { isFreeRight = false; }
		}
	}

	if (isFreeUp)
	{
		freePath.emplace_back(Direction::UP);
	}

	if (isFreeDown)
	{
		freePath.emplace_back(Direction::DOWN);
	}

	if (isFreeLeft)
	{
		freePath.emplace_back(Direction::LEFT);
	}

	if (isFreeRight)
	{
		freePath.emplace_back(Direction::RIGHT);
	}

	return freePath;
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

bool MoveLikeTankBeh::Move(const double deltaTime)
{
	const auto currentDirection = _direction;
	if (currentDirection == Direction::UP)
	{
		return MoveUp(deltaTime);
	}

	if (currentDirection == Direction::LEFT)
	{
		return MoveLeft(deltaTime);
	}

	if (currentDirection == Direction::DOWN)
	{
		return MoveDown(deltaTime);
	}

	if (currentDirection == Direction::RIGHT)
	{
		return MoveRight(deltaTime);
	}

	return false;
}

bool MoveLikeTankBeh::MoveLeft(const double deltaTime)
{
	if (const float speed = _speed * static_cast<float>(deltaTime); _rect.x - speed >= 0.f)
	{
		if (IsCanMove(deltaTime))
		{
			_rect.x -= std::floor(speed);

			return true;
		}

		// move less than speed to stand next to an object
		const auto getSideDiff = [thisLeftSide = _rect.x](const std::shared_ptr<BaseObj>& object) -> float
		{
			return thisLeftSide - object->GetRightSide();
		};

		constexpr float padding = 1.f;
		if (const float distance = FindMinDistance(_touchedObstacles, getSideDiff) - padding; distance > 0.f)
		{
			_rect.x -= std::floor(distance);

			return true;
		}

		HandleBonusPickUp(_touchedObstacles.front());
		_touchedObstacles.clear();
	}

	return false;
}

bool MoveLikeTankBeh::MoveRight(const double deltaTime)
{
	constexpr int sideBarWidth = 175;//TODO: pass this as parameter in constructor
	const float maxX = static_cast<float>(_windowSize.x) - sideBarWidth;
	if (const float speed = _speed * static_cast<float>(deltaTime); _rect.Right() + speed < maxX)
	{
		if (IsCanMove(deltaTime))
		{
			_rect.x += std::floor(speed);

			return true;
		}

		// move less than speed to stand next to an object
		auto getSideDiff = [thisRightSide = _rect.Right()](const std::shared_ptr<BaseObj>& object) -> float
		{
			return object->GetX() - thisRightSide;
		};

		constexpr float padding = 1.f;
		if (const float distance = FindMinDistance(_touchedObstacles, getSideDiff) - padding; distance > 0.f)
		{
			_rect.x += std::floor(distance);

			return true;
		}

		HandleBonusPickUp(_touchedObstacles.front());
		_touchedObstacles.clear();
	}

	return false;
}

bool MoveLikeTankBeh::MoveUp(const double deltaTime)
{
	if (const float speed = _speed * static_cast<float>(deltaTime); _rect.y - speed >= 0.0f)
	{
		if (IsCanMove(deltaTime))
		{
			_rect.y -= std::floor(speed);

			return true;
		}

		// move less than speed to stand next to an object
		const auto& getSideDiff = [thisTopSide = _rect.y](const std::shared_ptr<BaseObj>& object) -> float
		{
			return object->GetBottomSide() - thisTopSide;
		};

		constexpr float padding = 1.f;
		if (const float distance = FindMinDistance(_touchedObstacles, getSideDiff) - padding; distance > 0.f)
		{
			_rect.y -= std::floor(distance);

			return true;
		}

		HandleBonusPickUp(_touchedObstacles.front());
		_touchedObstacles.clear();
	}

	return false;
}

bool MoveLikeTankBeh::MoveDown(const double deltaTime)
{
	if (const float speed = _speed * static_cast<float>(deltaTime);
		_rect.Bottom() + speed < static_cast<float>(_windowSize.y))
	{
		if (IsCanMove(deltaTime))
		{
			_rect.y += std::floor(speed);

			return true;
		}

		// move less than speed to stand next to an object
		const auto getSideDiff =
				[thisBottomSide = _rect.Bottom()](const std::shared_ptr<BaseObj>& object) -> float
		{
			return object->GetY() - thisBottomSide;
		};

		constexpr float padding = 1.f;
		if (const float distance = FindMinDistance(_touchedObstacles, getSideDiff) - padding; distance > 0.f)
		{
			_rect.y += std::floor(distance);

			return true;
		}

		HandleBonusPickUp(_touchedObstacles.front());
		_touchedObstacles.clear();
	}

	return false;
}

//TODO: move method outside beh to tank class
void MoveLikeTankBeh::HandleBonusPickUp(const std::shared_ptr<BaseObj>& object) const
{
	if (const auto bonus = dynamic_cast<IPickupableBonus*>(object.get()))
	{
		bonus->PickUpBonus(_name, _fraction);
		//TODO: destroy bonus on emit in PickUpBonus by subscription
		object->TakeDamage(1);
	}
}
