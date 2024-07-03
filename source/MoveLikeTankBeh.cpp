#include "../headers/MoveLikeTankBeh.h"
#include "../headers/Tank.h"

#include <functional>
#include <memory>

MoveLikeTankBeh::MoveLikeTankBeh(BaseObj* selfParent, std::vector<std::shared_ptr<BaseObj>>* allObjects)
	: _selfParent{selfParent}, _allObjects{allObjects} {}

bool MoveLikeTankBeh::IsCollideWith(const Rectangle& r1, const Rectangle& r2)
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

std::list<std::weak_ptr<BaseObj>> MoveLikeTankBeh::IsCanMove(const float deltaTime) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return std::list<std::weak_ptr<BaseObj>>();
	}

	float speed = tank->GetSpeed();
	float speedX = speed * deltaTime;
	float speedY = speed * deltaTime;

	if (Direction direction = tank->GetDirection();
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
	const auto thisNextPosRect = Rectangle{_selfParent->GetX() + speedX, _selfParent->GetY() + speedY,
	                                       _selfParent->GetWidth(), _selfParent->GetHeight()};
	for (std::shared_ptr<BaseObj>& pawn: *_allObjects)
	{
		if (_selfParent == pawn.get())
		{
			continue;
		}

		if (IsCollideWith(thisNextPosRect, pawn->GetShape()))
		{
			if (!pawn->GetIsPassable())
			{
				obstacles.emplace_back(std::weak_ptr(pawn));
			}
		}
	}

	return obstacles;
}

inline float Distance(const FPoint a, const FPoint b)
{
	return static_cast<float>(std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2)));
}

float MoveLikeTankBeh::FindMinDistance(const std::list<std::weak_ptr<BaseObj>>& pawns,
                                       const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return 0.f;
	}

	float minDist = static_cast<float>(tank->GetWindowSize().x * tank->GetWindowSize().y);
	// float nearestDist = 0;
	for (const auto& pawn: pawns)
	{
		const std::shared_ptr<BaseObj> pawnLck = pawn.lock();
		// auto getSide = [](const std::shared_ptr<BaseObj>& pawnLck) -> float { return pawnLck->GetX() + pawnLck->GetWidth();};
		const float distance = std::abs(sideDiff(pawnLck));
		// const float distance = abs(this->GetX() - pawnLck->GetX() + pawnLck->GetWidth());
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

void MoveLikeTankBeh::Move(float deltaTime) const
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

void MoveLikeTankBeh::MoveLeft(const float deltaTime) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return;
	}

	if (const float speed = tank->GetSpeed() * deltaTime; _selfParent->GetX() - speed >= 0.f)
	{
		if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
		{
			_selfParent->MoveX(-std::floor(speed));
		}
		else
		{
			const auto getSideDiff = [thisLeftSide = _selfParent->GetX()](const std::shared_ptr<BaseObj>& pawn) -> float
			{
				return thisLeftSide - pawn->GetRightSide();
			};

			constexpr float padding = 1.f;
			if (const float distance = FindMinDistance(pawns, getSideDiff) - padding; distance > 0.f)
			{
				_selfParent->MoveX(-std::floor(distance));
			}
		}
	}
}

void MoveLikeTankBeh::MoveRight(const float deltaTime) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return;
	}

	constexpr int sideBarWidth = 175;//TODO: pass this as parameter in constructor
	const float maxX = static_cast<float>(tank->GetWindowSize().x) - sideBarWidth;
	if (const float speed = tank->GetSpeed() * deltaTime; _selfParent->GetRightSide() + speed < maxX)
	{
		if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
		{
			_selfParent->MoveX(std::floor(speed));
		}
		else
		{
			auto getSideDiff = [thisRightSide =
						_selfParent->GetRightSide()](const std::shared_ptr<BaseObj>& pawn) -> float
			{
				return pawn->GetX() - thisRightSide;
			};

			constexpr float padding = 1.f;
			if (const float distance = FindMinDistance(pawns, getSideDiff) - padding; distance > 0.f)
			{
				_selfParent->MoveX(std::floor(distance));
			}
		}
	}
}

void MoveLikeTankBeh::MoveUp(const float deltaTime) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return;
	}

	if (const float speed = tank->GetSpeed() * deltaTime; _selfParent->GetY() - speed >= 0.0f)
	{
		if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
		{
			_selfParent->MoveY(-std::floor(speed));
		}
		else
		{
			const auto& getSideDiff = [thisTopSide = _selfParent->GetY()](const std::shared_ptr<BaseObj>& pawn) -> float
			{
				return pawn->GetBottomSide() - thisTopSide;
			};

			constexpr float padding = 1.f;
			if (const float distance = FindMinDistance(pawns, getSideDiff) - padding; distance > 0.f)
			{
				_selfParent->MoveY(-std::floor(distance));
			}
		}
	}
}

void MoveLikeTankBeh::MoveDown(const float deltaTime) const
{
	const auto tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return;
	}

	if (const float speed = tank->GetSpeed() * deltaTime;
		_selfParent->GetBottomSide() + speed < static_cast<float>(tank->GetWindowSize().y))
	{
		if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
		{
			_selfParent->MoveY(std::floor(speed));
		}
		else
		{
			const auto getSideDiff =
					[thisBottomSide = _selfParent->GetBottomSide()](const std::shared_ptr<BaseObj>& pawn) -> float
			{
				return pawn->GetY() - thisBottomSide;
			};

			constexpr float padding = 1.f;
			if (const float distance = FindMinDistance(pawns, getSideDiff) - padding; distance > 0.f)
			{
				_selfParent->MoveY(std::floor(distance));
			}
		}
	}
}
