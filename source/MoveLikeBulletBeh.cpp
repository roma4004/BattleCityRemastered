#include "../headers/MoveLikeBulletBeh.h"

#include <functional>
#include <memory>

MoveLikeBulletBeh::MoveLikeBulletBeh(UPoint windowSize, float speed, int damage, BaseObj* selfParent,
                                     std::vector<std::shared_ptr<BaseObj>>* allPawns)
	: _windowSize(windowSize), _selfParent{selfParent}, _speed{speed}, _allPawns{allPawns}, _damage{damage} {}

bool MoveLikeBulletBeh::IsCollideWith(const Rectangle& r1, const Rectangle& r2)
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

inline float Distance(const FPoint a, const FPoint b)
{
	return static_cast<float>(std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2)));
}

float MoveLikeBulletBeh::FindNearestDistance(const std::list<std::weak_ptr<BaseObj>>& pawns,
                                             const std::function<float(const std::shared_ptr<BaseObj>&)>&
                                             getNearestSide)
const
{
	float nearestDist = static_cast<float>(_windowSize.x * _windowSize.y);
	// float nearestDist = 0;
	for (const auto& pawn: pawns)
	{
		const std::shared_ptr<BaseObj> pawnLck = pawn.lock();
		// auto getSide = [](const std::shared_ptr<BaseObj>& pawnLck) -> float { return pawnLck->GetX() + pawnLck->GetWidth();};
		const float distance = std::abs(getNearestSide(pawnLck));
		// const float distance = abs(this->GetX() - pawnLck->GetX() + pawnLck->GetWidth());
		if (distance < nearestDist)//TODO: need minimal abs distance
		{
			nearestDist = distance;
		}
	}

	return nearestDist;

	// constexpr auto padding = 1.f;
	// float distance = this->GetX() - nearestX - padding;
	// if (distance < padding)
	// {
	// 	return 0.f;
	// }
	//
	// return distance;
}

void MoveLikeBulletBeh::MoveLeft(const float deltaTime) const
{
	if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
	{
		_selfParent->MoveX(-_speed * deltaTime);
	}
	else
	{
		DealDamage(pawns);
	}
}

void MoveLikeBulletBeh::MoveRight(const float deltaTime) const
{
	if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
	{
		_selfParent->MoveX(_speed * deltaTime);
	}
	else
	{
		DealDamage(pawns);
	}
}

void MoveLikeBulletBeh::MoveUp(const float deltaTime) const
{
	if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
	{
		_selfParent->MoveY(-_speed * deltaTime);
	}
	else
	{
		DealDamage(pawns);
	}
}

void MoveLikeBulletBeh::MoveDown(const float deltaTime) const
{
	if (const auto pawns = IsCanMove(deltaTime); pawns.empty())
	{
		_selfParent->MoveY(_speed * deltaTime);
	}
	else
	{
		DealDamage(pawns);
	}
}

bool MoveLikeBulletBeh::CheckIntersection(const Circle& circle, const Rectangle& rect)
{
	const double deltaX = circle.center.x - std::max(rect.x, std::min(circle.center.x, rect.Right()));
	const double deltaY = circle.center.y - std::max(rect.y, std::min(circle.center.y, rect.Bottom()));

	return (deltaX * deltaX + deltaY * deltaY) < (circle.radius * circle.radius);
}

std::list<std::weak_ptr<BaseObj>> MoveLikeBulletBeh::IsCanMove(const float deltaTime) const
{
	float speedX = GetSpeed() * deltaTime;
	float speedY = GetSpeed() * deltaTime;

	// For some reason I can't make rect1 in if's Rider say I make unused object. So I made more crutches
	if (_direction == UP)
	{
		//36 37 initialize in if
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

	std::list<std::weak_ptr<BaseObj>> aoeList{};
	const auto bulletNextPosRect = Rectangle{_selfParent->GetX() + speedX, _selfParent->GetY() + speedY,
	                                         _selfParent->GetWidth(), _selfParent->GetHeight()};
	for (const std::shared_ptr<BaseObj>& pawn: *_allPawns)
	{
		if (_selfParent == pawn.get())
		{
			continue;
		}

		if (IsCollideWith(bulletNextPosRect, pawn->GetShape()))
		{
			if (!pawn->GetIsPenetrable())
			{
				//CheckAoE(_env, deltaTime, aoeList);
				CheckCircleAoE(FPoint{_selfParent->GetX() + speedX, _selfParent->GetY() + speedY}, aoeList);
				return aoeList;
			}
		}
	}

	return aoeList;
}

void MoveLikeBulletBeh::CheckCircleAoE(const FPoint blowCenter, std::list<std::weak_ptr<BaseObj>>& aoeList) const
{
	const Circle circle{blowCenter, 12};
	for (const std::shared_ptr<BaseObj>& pawn: *_allPawns)
	{
		if (_selfParent == pawn.get())
		{
			continue;
		}

		if (CheckIntersection(circle, pawn->GetShape()))
		{
			aoeList.emplace_back(std::weak_ptr(pawn));
		}
	}
}

void MoveLikeBulletBeh::CheckAoE(const float deltaTime, std::list<std::weak_ptr<BaseObj>>& aoeList) const
{
	const float x = _selfParent->GetX();
	const float y = _selfParent->GetY();
	const float width = _selfParent->GetWidth();
	const float height = _selfParent->GetHeight();
	const float speed = (GetSpeed() * deltaTime) * 2;

	for (const std::list<Rectangle> targetList{
			     // NOTE: targets ordered in numpad positions
			     /*1*/ {x - speed, y - speed, width, height},
			           /*2*/ {x, y - speed, width, height},
			           /*3*/ {x + speed, y - speed, width, height},

			           /*4*/ {x - speed, y, width, height},
			           /*5*/ {x, y, width, height},// bullet position
			           /*6*/ {x + speed, y, width, height},

			           /*7*/ {x - speed, y + speed, width, height},
			           /*8*/ {x, y + speed, width, height},
			           /*9*/ {x + speed, y + speed, width, height}};
	     auto target: targetList)
	{
		for (const std::shared_ptr<BaseObj>& pawn: *_allPawns)
		{
			if (_selfParent == pawn.get())
			{
				continue;
			}

			if (IsCollideWith(target, pawn->GetShape()))
			{
				aoeList.emplace_back(std::weak_ptr(pawn));
			}
		}
	}
}

void MoveLikeBulletBeh::DealDamage(const std::list<std::weak_ptr<BaseObj>>& objectList) const
{
	if (!objectList.empty())
	{
		for (const auto& target: objectList)
		{
			if (const auto targetLock = target.lock(); targetLock && targetLock->GetIsDestructible())
			{
				targetLock->TakeDamage(_damage);
			}
		}
	}

	_selfParent->TakeDamage(_damage);
}