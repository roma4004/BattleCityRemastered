#include "../headers/MoveLikeBulletBeh.h"
#include "../headers/Bullet.h"
#include "../headers/Circle.h"
#include "../headers/Direction.h"

#include <memory>

MoveLikeBulletBeh::MoveLikeBulletBeh(BaseObj* parent, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                                     std::shared_ptr<EventSystem> events)
	: _selfParent{parent},
	  _allObjects{allObjects},
	  _events{std::move(events)} {}

inline bool IsCollideWith(const Rectangle& r1, const Rectangle& r2)
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

std::list<std::weak_ptr<BaseObj>> MoveLikeBulletBeh::IsCanMove(const float deltaTime) const
{
	const auto bullet = dynamic_cast<Bullet*>(_selfParent);
	if (bullet == nullptr)
	{
		return std::list<std::weak_ptr<BaseObj>>();
	}

	float speed = bullet->GetSpeed();
	float speedX = speed * deltaTime;
	float speedY = speed * deltaTime;

	// For some reason I can't make rect1 in if's Rider say I make unused object. So I made more crutches
	if (const Direction direction = bullet->GetDirection();
		direction == UP)
	{
		//36 37 initialize in if
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

	std::list<std::weak_ptr<BaseObj>> aoeList{};
	const auto bulletNextPosRect = Rectangle{bullet->GetX() + speedX, bullet->GetY() + speedY,
	                                         bullet->GetWidth(), bullet->GetHeight()};
	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (bullet == object.get())
		{
			continue;
		}

		if (IsCollideWith(bulletNextPosRect, object->GetShape()))
		{
			if (!object->GetIsPenetrable())
			{
				CheckCircleAoE(FPoint{bullet->GetX() + speedX, bullet->GetY() + speedY}, aoeList);
				return aoeList;
			}
		}
	}

	return aoeList;
}

void MoveLikeBulletBeh::Move(const float deltaTime) const
{
	const auto bullet = dynamic_cast<Bullet*>(_selfParent);
	if (bullet == nullptr)
	{
		return;
	}

	constexpr int sideBarWidth = 175;
	const float speed = bullet->GetSpeed() * deltaTime;
	if (const int direction = bullet->GetDirection(); direction == UP && bullet->GetY() - speed >= 0.0f)
	{
		MoveUp(deltaTime);
	}
	else if (direction == DOWN && bullet->GetBottomSide() + speed <= static_cast<float>(bullet->GetWindowSize().y))
	{
		MoveDown(deltaTime);
	}
	else if (direction == LEFT && bullet->GetX() - speed >= 0.0f)
	{
		MoveLeft(deltaTime);
	}
	else if (direction == RIGHT && bullet->GetRightSide() + speed <= static_cast<float>(bullet->GetWindowSize().x)
	         - sideBarWidth)
	{
		MoveRight(deltaTime);
	}
	else// Self-destroy when edge of windows is reached
	{
		bullet->SetIsAlive(false);
	}
}

void MoveLikeBulletBeh::MoveLeft(const float deltaTime) const
{
	if (const auto objects = IsCanMove(deltaTime); objects.empty())
	{
		const auto bullet = dynamic_cast<Bullet*>(_selfParent);
		if (bullet == nullptr)
		{
			return;
		}

		bullet->MoveX(-bullet->GetSpeed() * deltaTime);
	}
	else
	{
		DealDamage(objects);
	}
}

void MoveLikeBulletBeh::MoveRight(const float deltaTime) const
{
	if (const auto objects = IsCanMove(deltaTime); objects.empty())
	{
		const auto bullet = dynamic_cast<Bullet*>(_selfParent);
		if (bullet == nullptr)
		{
			return;
		}

		bullet->MoveX(bullet->GetSpeed() * deltaTime);
	}
	else
	{
		DealDamage(objects);
	}
}

void MoveLikeBulletBeh::MoveUp(const float deltaTime) const
{
	if (const auto objects = IsCanMove(deltaTime); objects.empty())
	{
		const auto bullet = dynamic_cast<Bullet*>(_selfParent);
		if (bullet == nullptr)
		{
			return;
		}

		bullet->MoveY(-bullet->GetSpeed() * deltaTime);
	}
	else
	{
		DealDamage(objects);
	}
}

void MoveLikeBulletBeh::MoveDown(const float deltaTime) const
{
	if (const auto objects = IsCanMove(deltaTime); objects.empty())
	{
		const auto bullet = dynamic_cast<Bullet*>(_selfParent);
		if (bullet == nullptr)
		{
			return;
		}

		bullet->MoveY(bullet->GetSpeed() * deltaTime);
	}
	else
	{
		DealDamage(objects);
	}
}

inline bool CheckIntersection(const Circle& circle, const Rectangle& rect)
{
	const double deltaX = circle.center.x - std::max(rect.x, std::min(circle.center.x, rect.Right()));
	const double deltaY = circle.center.y - std::max(rect.y, std::min(circle.center.y, rect.Bottom()));

	return (deltaX * deltaX + deltaY * deltaY) < (circle.radius * circle.radius);
}

void MoveLikeBulletBeh::CheckCircleAoE(const FPoint blowCenter, std::list<std::weak_ptr<BaseObj>>& aoeList) const
{
	const auto bullet = dynamic_cast<Bullet*>(_selfParent);
	if (bullet == nullptr)
	{
		return;
	}

	const Circle circle{blowCenter, bullet->GetBulletDamageAreaRadius()};
	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (_selfParent == object.get())
		{
			continue;
		}

		if (CheckIntersection(circle, object->GetShape()))
		{
			aoeList.emplace_back(std::weak_ptr(object));
		}
	}
}

void MoveLikeBulletBeh::DealDamage(const std::list<std::weak_ptr<BaseObj>>& objectList) const
{
	const auto thisBullet = dynamic_cast<Bullet*>(_selfParent);
	if (thisBullet == nullptr)
	{
		return;
	}

	const int bulletDamage = thisBullet->GetDamage();
	if (!objectList.empty())
	{
		for (const auto& target: objectList)
		{
			if (const std::shared_ptr<BaseObj> targetLock = target.lock();
				targetLock && targetLock->GetIsDestructible())
			{
				targetLock->TakeDamage(bulletDamage);
				targetLock->SendDamageStatistics(thisBullet->GetAuthor(), thisBullet->GetFraction());
				if (const auto otherBullet = dynamic_cast<Bullet*>(targetLock.get()))
				{
					thisBullet->SendDamageStatistics(otherBullet->GetAuthor(), otherBullet->GetFraction());
				}
			}
		}
	}

	thisBullet->TakeDamage(bulletDamage);
}
