#include "../headers/MoveLikeAIBeh.h"

#include <functional>
#include <memory>

MoveLikeAIBeh::MoveLikeAIBeh(const Direction direction, const UPoint windowSize, const float speed, BaseObj* selfParent,
                             std::vector<std::shared_ptr<BaseObj>>* allPawns)
	: _windowSize(windowSize), _selfParent{selfParent}, _direction{direction}, _speed{speed}, _allPawns{allPawns} {}

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
	float speedX = _speed * deltaTime;
	float speedY = _speed * deltaTime;

	if (_direction == UP)
	{
		//36 37 initialize in  if
		speedY *= -1;
		speedX *= 0;
	}
	else if (_direction == DOWN)
	{
		speedY *= 1;
		speedX *= 0;
	}
	else if (_direction == LEFT)
	{
		speedX *= -1;
		speedY *= 0;
	}
	else if (_direction == RIGHT)
	{
		speedX *= 1;
		speedY *= 0;
	}

	std::list<std::weak_ptr<BaseObj>> obstacles{};
	const auto thisNextPosRect = Rectangle{_selfParent->GetX() + speedX, _selfParent->GetY() + speedY,
	                                       _selfParent->GetWidth(), _selfParent->GetHeight()};
	for (std::shared_ptr<BaseObj>& pawn: *_allPawns)
	{
		if (_selfParent == pawn.get())
		{
			continue;
		}

		if (IsCollideWith(thisNextPosRect, pawn->GetShape()))
		{
			if (!pawn->GetIsPassable())
			{
				//TODO: need fix, we broke collision detecting
				obstacles.emplace_back(std::weak_ptr(pawn));
				// return std::make_tuple(false, obstacle);
			}
			//else
			//{
			//	obstacle.emplace_back(std::weak_ptr(pawn));
			//	isHaveCollision = true;
			//	return std::make_tuple(true, obstacle);
			//}
		}
	}

	return obstacles;
}

inline float Distance(const FPoint a, const FPoint b)
{
	return static_cast<float>(std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2)));
}

float MoveLikeAIBeh::FindMinDistance(const std::list<std::weak_ptr<BaseObj>>& pawns,
                                     const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const
{
	float minDist = static_cast<float>(_windowSize.x * _windowSize.y);
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

void MoveLikeAIBeh::MoveLeft(const float deltaTime) const
{
	if (const float speed = _speed * deltaTime; _selfParent->GetX() - speed >= 0.f)
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

void MoveLikeAIBeh::MoveRight(const float deltaTime) const
{
	constexpr int sideBarWidth = 175;
	const float maxX = static_cast<float>(_windowSize.x) - sideBarWidth;
	if (const float speed = _speed * deltaTime; _selfParent->GetRightSide() + speed < maxX)
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

void MoveLikeAIBeh::MoveUp(const float deltaTime) const
{
	if (const float speed = _speed * deltaTime; _selfParent->GetY() - speed >= 0.0f)
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

void MoveLikeAIBeh::MoveDown(const float deltaTime) const
{
	if (const float speed = _speed * deltaTime;
		_selfParent->GetBottomSide() + speed < static_cast<float>(_windowSize.y))
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
