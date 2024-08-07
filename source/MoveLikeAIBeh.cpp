#include "../headers/MoveLikeAIBeh.h"
#include "../headers/Tank.h"

#include <functional>
#include <memory>

MoveLikeAIBeh::MoveLikeAIBeh(BaseObj* selfParent, std::vector<std::shared_ptr<BaseObj>>* allObjects)
	: _selfParent{selfParent}, _allObjects{allObjects} {}

bool MoveLikeAIBeh::IsCollideWith(const Rectangle& r1, const Rectangle& r2)
{

	// Check if one rectangle is to the right of the other
	if (r1.x > r2.x + r2.w || r2.x > r1.x + r1.w)
	{
		return false;
	}

	// Check if one rectangle is above the other
	if (r1.y > r2.y + r2.h || r2.y > r1.y + r1.h)
	{
		return false;
	}

	// If neither of the above conditions are met, the rectangles overlap
	return true;
}

std::list<std::weak_ptr<BaseObj>> MoveLikeAIBeh::IsCanMove(const float deltaTime) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return std::list<std::weak_ptr<BaseObj>>();
	}

	float speed = tank->GetSpeed();
	float speedX = speed * deltaTime;
	float speedY = speed * deltaTime;

	if (const Direction direction = tank->GetDirection();
		direction == UP)
	{
		//36 37 initialize in  if
		speedY *= -1;
		speedX *= 0;
	}
	else if (direction == DOWN)
	{
		speedY *= 1;
		speedX *= 0;
	}
	else if (direction == LEFT)
	{
		speedX *= -1;
		speedY *= 0;
	}
	else if (direction == RIGHT)
	{
		speedX *= 1;
		speedY *= 0;
	}

	std::list<std::weak_ptr<BaseObj>> obstacles{};
	const auto thisNextPosRect = Rectangle{tank->GetX() + speedX, tank->GetY() + speedY,
	                                       tank->GetWidth(), tank->GetHeight()};
	for (std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (tank == object.get())
		{
			continue;
		}

		if (IsCollideWith(thisNextPosRect, object->GetShape()))
		{
			if (!object->GetIsPassable())
			{
				obstacles.emplace_back(std::weak_ptr(object));
			}
		}
	}

	return obstacles;
}

inline float Distance(const FPoint a, const FPoint b)
{
	return static_cast<float>(std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2)));
}

float MoveLikeAIBeh::FindMinDistance(const std::list<std::weak_ptr<BaseObj>>& objects,
                                     const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return 0.f;
	}

	UPoint windowSize = tank->GetWindowSize();
	float minDist = static_cast<float>(windowSize.x * windowSize.y);
	// float nearestDist = 0;
	for (const auto& object: objects)
	{
		const std::shared_ptr<BaseObj> objectLck = object.lock();
		// auto getSide = [](const std::shared_ptr<BaseObj>& objectLck) -> float { return objectLck->GetX() + objectLck->GetWidth();};
		const float distance = std::abs(sideDiff(objectLck));
		// const float distance = abs(this->GetX() - objectLck->GetX() + objectLck->GetWidth());
		if (distance < minDist)//TODO: need minimal abs distance
		{
			minDist = distance;
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

void MoveLikeAIBeh::Move(float deltaTime) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return;
	}

	if (const auto currentDirection = tank->GetDirection();
		currentDirection == UP)
	{
		MoveUp(deltaTime);
	}
	else if (currentDirection == LEFT)
	{
		MoveLeft(deltaTime);
	}
	else if (currentDirection == DOWN)
	{
		MoveDown(deltaTime);
	}
	else if (currentDirection == RIGHT)
	{
		MoveRight(deltaTime);
	}
}

void MoveLikeAIBeh::MoveLeft(const float deltaTime) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return;
	}

	if (const float speed = tank->GetSpeed() * deltaTime; tank->GetX() - speed >= 0.f)
	{
		if (const auto objects = IsCanMove(deltaTime); objects.empty())
		{
			tank->MoveX(-std::floor(speed));
		}
		else
		{
			const auto getSideDiff = [thisLeftSide = tank->GetX()](const std::shared_ptr<BaseObj>& object) -> float
			{
				return thisLeftSide - object->GetRightSide();
			};

			constexpr float padding = 1.f;
			if (const float distance = FindMinDistance(objects, getSideDiff) - padding; distance > 0.f)
			{
				tank->MoveX(-std::floor(distance));
			}
		}
	}
}

void MoveLikeAIBeh::MoveRight(const float deltaTime) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return;
	}

	constexpr int sideBarWidth = 175;
	const float maxX = static_cast<float>(tank->GetWindowSize().x) - sideBarWidth;
	if (const float speed = tank->GetSpeed() * deltaTime; tank->GetRightSide() + speed < maxX)
	{
		if (const auto objects = IsCanMove(deltaTime); objects.empty())
		{
			tank->MoveX(std::floor(speed));
		}
		else
		{
			auto getSideDiff = [thisRightSide = tank->GetRightSide()](const std::shared_ptr<BaseObj>& object) -> float
			{
				return object->GetX() - thisRightSide;
			};

			constexpr float padding = 1.f;
			if (const float distance = FindMinDistance(objects, getSideDiff) - padding; distance > 0.f)
			{
				tank->MoveX(std::floor(distance));
			}
		}
	}
}

void MoveLikeAIBeh::MoveUp(const float deltaTime) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return;
	}

	if (const float speed = tank->GetSpeed() * deltaTime; tank->GetY() - speed >= 0.0f)
	{
		if (const auto objects = IsCanMove(deltaTime); objects.empty())
		{
			tank->MoveY(-std::floor(speed));
		}
		else
		{
			const auto& getSideDiff = [thisTopSide = tank->GetY()](const std::shared_ptr<BaseObj>& object) -> float
			{
				return object->GetBottomSide() - thisTopSide;
			};

			constexpr float padding = 1.f;
			if (const float distance = FindMinDistance(objects, getSideDiff) - padding; distance > 0.f)
			{
				tank->MoveY(-std::floor(distance));
			}
		}
	}
}

void MoveLikeAIBeh::MoveDown(const float deltaTime) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return;
	}

	if (const float speed = tank->GetSpeed() * deltaTime;
		tank->GetBottomSide() + speed < static_cast<float>(tank->GetWindowSize().y))
	{
		if (const auto objects = IsCanMove(deltaTime); objects.empty())
		{
			tank->MoveY(std::floor(speed));
		}
		else
		{
			const auto getSideDiff =
					[thisBottomSide = tank->GetBottomSide()](const std::shared_ptr<BaseObj>& object) -> float
			{
				return object->GetY() - thisBottomSide;
			};

			constexpr float padding = 1.f;
			if (const float distance = FindMinDistance(objects, getSideDiff) - padding; distance > 0.f)
			{
				tank->MoveY(std::floor(distance));
			}
		}
	}
}
