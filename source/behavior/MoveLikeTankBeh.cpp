#include "../../headers/behavior/MoveLikeTankBeh.h"
#include "../../headers/Point.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/interfaces/IPickupableBonus.h"
#include "../../headers/pawns/Tank.h"
#include "../../headers/utils/ColliderUtils.h"
#include <functional>
#include <memory>

///TODO: change selfParent to Tank to avoid check on each IsCanMove
MoveLikeTankBeh::MoveLikeTankBeh(BaseObj* selfParent, std::vector<std::shared_ptr<BaseObj>>* allObjects)
	: _selfParent{selfParent}, _allObjects{allObjects} {}

std::vector<std::shared_ptr<BaseObj>> MoveLikeTankBeh::IsCanMove(const float deltaTime) const
{
	const auto* tank = dynamic_cast<Tank*>(_selfParent);
	std::vector<std::shared_ptr<BaseObj>> obstacles{};
	constexpr int defaultCollisionReserve{5};
	obstacles.reserve(defaultCollisionReserve);
	if (tank == nullptr)
	{
		// TODO: assert component must be in tank class
		return obstacles;
	}

	const float speed = tank->GetSpeed();
	const float moveSpeed = speed * deltaTime;
	ObjRectangle tankNextPosRect;
	if (const Direction dir = tank->GetDirection();
		dir == UP)
	{
		tankNextPosRect = ObjRectangle{
				.x = tank->GetX(),
				.y = tank->GetY() - moveSpeed,
				.w = tank->GetWidth(),
				.h = tank->GetHeight() + moveSpeed
		};
	}
	else if (dir == DOWN)
	{
		tankNextPosRect = ObjRectangle{
				.x = tank->GetX(),
				.y = tank->GetY(),
				.w = tank->GetWidth(),
				.h = tank->GetHeight() + moveSpeed
		};
	}
	else if (dir == LEFT)
	{
		tankNextPosRect = ObjRectangle{
				.x = tank->GetX() - moveSpeed,
				.y = tank->GetY(),
				.w = tank->GetWidth() + moveSpeed,
				.h = tank->GetHeight()
		};
	}
	else if (dir == RIGHT)
	{
		tankNextPosRect = ObjRectangle{
				.x = tank->GetX(),
				.y = tank->GetY(),
				.w = tank->GetWidth() + moveSpeed,
				.h = tank->GetHeight()
		};
	}

	for (std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (object.get() == nullptr || tank == object.get())
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

// inline float Distance(const FPoint a, const FPoint b)
// {
// 	return static_cast<float>(std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2)));
// }

float MoveLikeTankBeh::FindMinDistance(const std::vector<std::shared_ptr<BaseObj>>& objects,
                                       const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const
{
	const auto* tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return 0.f;
	}

	const UPoint windowSize = tank->GetWindowSize();
	auto minDist = static_cast<float>(windowSize.x * windowSize.y);
	// float nearestDist = 0;
	for (const auto& object: objects)
	{
		if (object.get() != nullptr)
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
	if (currentDirection == UP)
	{
		return MoveUp(deltaTime);
	}
	if (currentDirection == LEFT)
	{
		return MoveLeft(deltaTime);
	}
	if (currentDirection == DOWN)
	{
		return MoveDown(deltaTime);
	}
	if (currentDirection == RIGHT)
	{
		return MoveRight(deltaTime);
	}

	return false;
}

bool MoveLikeTankBeh::MoveLeft(const float deltaTime) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
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
			// move less than speed to stand next to object
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
	const auto tank = dynamic_cast<Tank*>(_selfParent);
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
			// move less than speed to stand next to object
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
	const auto tank = dynamic_cast<Tank*>(_selfParent);
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
			// move less than speed to stand next to object
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
	const auto tank = dynamic_cast<Tank*>(_selfParent);
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
			// move less than speed to stand next to object
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
	if (const auto target = object)
	{
		if (const auto bonus = dynamic_cast<IPickupableBonus*>(target.get()))
		{
			bonus->PickUpBonus(tank->GetName(), tank->GetFraction());
			//TODO: destroy bonus on emit in PickUpBonus by subscription
			target->TakeDamage(1);
		}
	}
}
