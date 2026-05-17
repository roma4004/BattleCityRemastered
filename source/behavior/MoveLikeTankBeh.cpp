#include "behavior/MoveLikeTankBeh.h"
#include "Point.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "utils/ColliderUtils.h"
#include <algorithm>
#include <memory>
#include <ranges>

MoveLikeTankBeh::MoveLikeTankBeh(ObjRectangle& rect, Direction& dir, float& speed, buuid& uuid, UPoint& windowSize,
								 std::string& name, std::string& fraction,
								 std::vector<std::shared_ptr<BaseObj>>* allObjects)
	: _uuid{uuid}
	, _rect{rect}
	, _direction{dir}
	, _speed{speed}
	, _windowSize{windowSize}
	, _name{name}
	, _fraction{fraction}
	, _allObjects{allObjects} {}

ObjRectangle MoveLikeTankBeh::GetNextPosRect(const double deltaTime) const
{
	const float speed = _speed * static_cast<float>(deltaTime);//TODO: speed from float to double, as well as rectangle
	const auto [x, y, w, h] = _rect;
	if (_direction == Direction::UP)
	{
		return ObjRectangle{.x = x, .y = y - speed, .w = w, .h = h + speed};
	}

	if (_direction == Direction::DOWN)
	{
		return ObjRectangle{.x = x, .y = y, .w = w, .h = h + speed};
	}

	if (_direction == Direction::LEFT)
	{
		return ObjRectangle{.x = x - speed, .y = y, .w = w + speed, .h = h};
	}

	if (_direction == Direction::RIGHT)
	{
		return ObjRectangle{.x = x, .y = y, .w = w + speed, .h = h};
	}

	return ObjRectangle{};
}

bool MoveLikeTankBeh::IsCanMove(const double deltaTime) const
{
	const ObjRectangle tankNextPosRect = GetNextPosRect(deltaTime);

	return std::ranges::none_of(*_allObjects, [uuid = _uuid, tankNextPosRect](const std::shared_ptr<BaseObj>& object)
	{
		return uuid != object->GetUuid()
			   && ColliderUtils::IsCollide(tankNextPosRect, object->GetRect())
			   && !object->GetIsPassable();
	});
}

std::vector<std::shared_ptr<BaseObj>> MoveLikeTankBeh::GetTouchedObjects(const double deltaTime) const
{
	const ObjRectangle tankNextPosRect = GetNextPosRect(deltaTime);

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

std::vector<Direction> MoveLikeTankBeh::GetFreePathSides(const double deltaTime) const
{
	std::vector<Direction> freePath;

	constexpr int defaultCollisionReserve{4};
	freePath.reserve(defaultCollisionReserve);

	const float speed = _speed * static_cast<float>(deltaTime);
	const auto [x, y, w, h] = _rect;
	const ObjRectangle tankNextPosRectUp{.x = x, .y = y - speed, .w = w, .h = h + speed};
	const ObjRectangle tankNextPosRectDown{.x = x, .y = y, .w = w, .h = h + speed};
	const ObjRectangle tankNextPosRectLeft{.x = x - speed, .y = y, .w = w + speed, .h = h};
	const ObjRectangle tankNextPosRectRight{.x = x, .y = y, .w = w + speed, .h = h};

	bool isFreeUp{true};
	bool isFreeDown{true};
	bool isFreeLeft{true};
	bool isFreeRight{true};

	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
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

bool MoveLikeTankBeh::Move(std::vector<std::shared_ptr<BaseObj>>& outCollisions, const double deltaTime)
{
	const auto currentDirection = _direction;
	if (currentDirection == Direction::UP)
	{
		return MoveUp(outCollisions, deltaTime);
	}

	if (currentDirection == Direction::LEFT)
	{
		return MoveLeft(outCollisions, deltaTime);
	}

	if (currentDirection == Direction::DOWN)
	{
		return MoveDown(outCollisions, deltaTime);
	}

	if (currentDirection == Direction::RIGHT)
	{
		return MoveRight(outCollisions, deltaTime);
	}

	return false;
}

bool MoveLikeTankBeh::MoveLeft(std::vector<std::shared_ptr<BaseObj>>& outCollisions, const double deltaTime)
{
	if (float speed = _speed * static_cast<float>(deltaTime); _rect.x - speed >= 0.f)
	{
		constexpr float maxMoveStep = 8.0f;
		speed = std::min(speed, maxMoveStep);
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
		outCollisions = GetTouchedObjects(deltaTime);
		if (const float distance = FindMinDistance(outCollisions, getSideDiff) - padding; distance > 0.f)
		{
			_rect.x -= std::floor(distance);

			return true;
		}
	}

	return false;
}

bool MoveLikeTankBeh::MoveRight(std::vector<std::shared_ptr<BaseObj>>& outCollisions, const double deltaTime)
{
	constexpr int sideBarWidth = 175;//TODO: pass this as parameter in constructor
	const float maxX = static_cast<float>(_windowSize.x) - sideBarWidth;
	if (float speed = _speed * static_cast<float>(deltaTime); _rect.Right() + speed < maxX)
	{
		constexpr float maxMoveStep = 8.0f;
		speed = std::min(speed, maxMoveStep);
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
		outCollisions = GetTouchedObjects(deltaTime);
		if (const float distance = FindMinDistance(outCollisions, getSideDiff) - padding; distance > 0.f)
		{
			_rect.x += std::floor(distance);

			return true;
		}
	}

	return false;
}

bool MoveLikeTankBeh::MoveUp(std::vector<std::shared_ptr<BaseObj>>& outCollisions, const double deltaTime)
{
	if (float speed = _speed * static_cast<float>(deltaTime); _rect.y - speed >= 0.0f)
	{
		constexpr float maxMoveStep = 8.0f;
		speed = std::min(speed, maxMoveStep);
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
		outCollisions = GetTouchedObjects(deltaTime);
		if (const float distance = FindMinDistance(outCollisions, getSideDiff) - padding; distance > 0.f)
		{
			_rect.y -= std::floor(distance);

			return true;
		}
	}

	return false;
}

bool MoveLikeTankBeh::MoveDown(std::vector<std::shared_ptr<BaseObj>>& outCollisions, const double deltaTime)
{
	if (float speed = _speed * static_cast<float>(deltaTime);
		_rect.Bottom() + speed < static_cast<float>(_windowSize.y))
	{
		constexpr float maxMoveStep = 8.0f;
		speed = std::min(speed, maxMoveStep);
		if (IsCanMove(deltaTime))
		{
			_rect.y += std::floor(speed);

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
			_rect.y += std::floor(distance);

			return true;
		}
	}

	return false;
}
