#include "behavior/MoveLikeTankBeh.h"
#include "Point.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "interfaces/IPickupableBonus.h"
#include "utils/ColliderUtils.h"
#include <functional>
#include <memory>

///TODO: change selfParent to Tank to avoid check on each IsCanMove
MoveLikeTankBeh::MoveLikeTankBeh(BaseObj* selfParent, std::vector<std::shared_ptr<BaseObj>>* allObjects)
	: _selfParent{selfParent}, _allObjects{allObjects} {}

std::vector<std::shared_ptr<BaseObj>> MoveLikeTankBeh::IsCanMove(const float deltaTime) const
{
	std::vector<std::shared_ptr<BaseObj>> obstacles{};
	const auto* tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return obstacles;
	}

	constexpr int defaultCollisionReserve{5};
	obstacles.reserve(defaultCollisionReserve);

	const float speed = tank->GetSpeed();
	const float moveSpeed = speed * deltaTime;
	const auto [x, y, w, h] = tank->GetRect();
	ObjRectangle tankNextPosRect;
	if (const Direction dir = tank->GetDirection();
		dir == Direction::UP)
	{
		tankNextPosRect = {.x = x, .y = y - moveSpeed, .w = w, .h = h + moveSpeed};
	}
	else if (dir == Direction::DOWN)
	{
		tankNextPosRect = {.x = x, .y = y, .w = w, .h = h + moveSpeed};
	}
	else if (dir == Direction::LEFT)
	{
		tankNextPosRect = {.x = x - moveSpeed, .y = y, .w = w + moveSpeed, .h = h};
	}
	else if (dir == Direction::RIGHT)
	{
		tankNextPosRect = {.x = x, .y = y, .w = w + moveSpeed, .h = h};
	}

	for (std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (object == nullptr || tank == object.get())
		{
			continue;
		}

		if (ColliderUtils::IsCollide(tankNextPosRect, object->GetRect()))
		{
			if (!object->GetIsPassable())
			{
				obstacles.emplace_back(object);
			}
		}
	}

	return obstacles;
}

std::vector<Direction> MoveLikeTankBeh::GetFreePathSides(const float deltaTime) const
{
	std::vector<Direction> freePath;
	const auto* tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return freePath;
	}

	constexpr int defaultCollisionReserve{4};
	freePath.reserve(defaultCollisionReserve);

	const float speed = tank->GetSpeed();
	const float moveSpeed = speed * deltaTime;
	const auto [x, y, w, h] = tank->GetRect();
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
		if (object == nullptr || tank == object.get())
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
	const auto* tank = dynamic_cast<Tank*>(_selfParent);
	const auto [maxX, maxY] = tank->GetWindowSize();
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

bool MoveLikeTankBeh::Move(const float deltaTime) const
{
	const auto* tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return false;
	}

	const auto currentDirection = tank->GetDirection();
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

bool MoveLikeTankBeh::MoveLeft(const float deltaTime) const
{
	auto* tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return false;
	}

	if (const float speed = tank->GetSpeed() * deltaTime; tank->GetX() - speed >= 0.f)
	{
		if (const auto objects = IsCanMove(deltaTime); objects.empty())
		{
			tank->MoveX(-std::floor(speed));

			return true;
		}
		else
		{
			// move less than speed to stand next to an object
			const auto getSideDiff = [thisLeftSide = tank->GetX()](const std::shared_ptr<BaseObj>& object) -> float
			{
				return thisLeftSide - object->GetRightSide();
			};

			constexpr float padding = 1.f;
			if (const float distance = FindMinDistance(objects, getSideDiff) - padding; distance > 0.f)
			{
				tank->MoveX(-std::floor(distance));

				return true;
			}

			HandleBonusPickUp(objects.front(), tank);
		}
	}

	return false;
}

bool MoveLikeTankBeh::MoveRight(const float deltaTime) const
{
	auto* tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return false;
	}

	constexpr int sideBarWidth = 175;//TODO: pass this as parameter in constructor
	const float maxX = static_cast<float>(tank->GetWindowSize().x) - sideBarWidth;
	if (const float speed = tank->GetSpeed() * deltaTime; tank->GetRightSide() + speed < maxX)
	{
		if (const auto objects = IsCanMove(deltaTime); objects.empty())
		{
			tank->MoveX(std::floor(speed));

			return true;
		}
		else
		{
			// move less than speed to stand next to an object
			auto getSideDiff = [thisRightSide = tank->GetRightSide()](const std::shared_ptr<BaseObj>& object) -> float
			{
				return object->GetX() - thisRightSide;
			};

			constexpr float padding = 1.f;
			if (const float distance = FindMinDistance(objects, getSideDiff) - padding; distance > 0.f)
			{
				tank->MoveX(std::floor(distance));

				return true;
			}

			HandleBonusPickUp(objects.front(), tank);
		}
	}

	return false;
}

bool MoveLikeTankBeh::MoveUp(const float deltaTime) const
{
	auto* tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return false;
	}

	if (const float speed = tank->GetSpeed() * deltaTime; tank->GetY() - speed >= 0.0f)
	{
		if (const auto objects = IsCanMove(deltaTime); objects.empty())
		{
			tank->MoveY(-std::floor(speed));

			return true;
		}
		else
		{
			// move less than speed to stand next to an object
			const auto& getSideDiff = [thisTopSide = tank->GetY()](const std::shared_ptr<BaseObj>& object) -> float
			{
				return object->GetBottomSide() - thisTopSide;
			};

			constexpr float padding = 1.f;
			if (const float distance = FindMinDistance(objects, getSideDiff) - padding; distance > 0.f)
			{
				tank->MoveY(-std::floor(distance));

				return true;
			}

			HandleBonusPickUp(objects.front(), tank);
		}
	}

	return false;
}

bool MoveLikeTankBeh::MoveDown(const float deltaTime) const
{
	auto* tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return false;
	}

	if (const float speed = tank->GetSpeed() * deltaTime;
		tank->GetBottomSide() + speed < static_cast<float>(tank->GetWindowSize().y))
	{
		if (const auto objects = IsCanMove(deltaTime); objects.empty())
		{
			tank->MoveY(std::floor(speed));

			return true;
		}
		else
		{
			// move less than speed to stand next to an object
			const auto getSideDiff =
					[thisBottomSide = tank->GetBottomSide()](const std::shared_ptr<BaseObj>& object) -> float
			{
				return object->GetY() - thisBottomSide;
			};

			constexpr float padding = 1.f;
			if (const float distance = FindMinDistance(objects, getSideDiff) - padding; distance > 0.f)
			{
				tank->MoveY(std::floor(distance));

				return true;
			}

			HandleBonusPickUp(objects.front(), tank);
		}
	}

	return false;
}

void MoveLikeTankBeh::HandleBonusPickUp(const std::shared_ptr<BaseObj>& object, const Tank* tank)
{
	if (const auto bonus = dynamic_cast<IPickupableBonus*>(object.get()))
	{
		bonus->PickUpBonus(std::string(tank->GetName()), tank->GetFraction());
		//TODO: destroy bonus on emit in PickUpBonus by subscription
		object->TakeDamage(1);
	}
}
