#include "../../headers/behavior/MoveLikeBulletBeh.h"
#include "../../headers/Circle.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/obstacles/GrassTile.h"
#include "../../headers/obstacles/IceTile.h"
#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/pawns/Bullet.h"
#include "../../headers/utils/ColliderUtils.h"
#include <memory>

MoveLikeBulletBeh::MoveLikeBulletBeh(BaseObj* parent, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                                     std::shared_ptr<EventSystem> events)
	: _selfParent{parent},
	  _allObjects{allObjects},
	  _events{std::move(events)} {}

ObjRectangle MoveLikeBulletBeh::GetBulletPathRect(const Bullet* bullet, const float deltaTime)
{
	const auto dir = bullet->GetDirection();
	const float speed = bullet->GetSpeed() * deltaTime;
	const auto [x, y, w, h] = bullet->GetRect();
	if (dir == UP)
	{
		return {.x = x, .y = y - speed, .w = w, .h = h + speed};
	}

	if (dir == DOWN)
	{
		return {.x = x, .y = y, .w = w, .h = h + speed};
	}

	if (dir == LEFT)
	{
		//TODO: write bullet test that can damage tank from all sides
		return {.x = x - speed, .y = y, .w = w + speed, .h = h};
	}

	//dir == RIGHT
	return {.x = x, .y = y, .w = w + speed, .h = h};
}

FPoint MoveLikeBulletBeh::GetBulletNextPoint(const Bullet* bullet, const float deltaTime)
{
	const auto dir = bullet->GetDirection();
	const float speed = bullet->GetSpeed() * deltaTime;
	const auto [x, y, w, h] = bullet->GetRect();
	if (dir == UP)
	{
		return {.x = x, .y = y - speed};
	}

	if (dir == DOWN)
	{
		return {.x = x, .y = y + speed};
	}

	if (dir == LEFT)
	{
		return {.x = x - speed, .y = y};//TODO: write bullet test that can damage tank from all sides
	}

	//dir == RIGHT
	return {.x = x + speed, .y = y};
}

std::vector<std::shared_ptr<BaseObj>> MoveLikeBulletBeh::IsCanMove(const float deltaTime) const
{
	const auto* bullet = dynamic_cast<Bullet*>(_selfParent);
	if (bullet == nullptr)
	{
		return {};
	}

	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (object.get() == nullptr || bullet == object.get())
		{
			continue;
		}

		if (ColliderUtils::IsCollide(GetBulletPathRect(bullet, deltaTime), object->GetRect()))
		{
			if (!object->GetIsPenetrable())
			{
				return GetCircleCollisionObjects(GetBulletNextPoint(bullet, deltaTime));
				//TODO: fix move to a bonus though water
			}
		}
	}

	return {};
}

bool MoveLikeBulletBeh::Move(const float deltaTime) const
{
	const auto bullet = dynamic_cast<Bullet*>(_selfParent);
	if (bullet == nullptr)
	{
		return false;
	}

	const float speed = bullet->GetSpeed() * deltaTime;
	const int direction = bullet->GetDirection();
	if (direction == UP && bullet->GetY() - speed >= 0.0f)
	{
		return MoveUp(deltaTime);
	}

	if (direction == DOWN && bullet->GetBottomSide() + speed <= static_cast<float>(bullet->GetWindowSize().y))
	//TODO: pass _window to movelikeBullet instead of bullet
	{
		return MoveDown(deltaTime);
	}

	if (direction == LEFT && bullet->GetX() - speed >= 0.0f)
	{
		return MoveLeft(deltaTime);
	}

	if (constexpr int sideBarWidth = 175;//TODO: move sidebar width to params
		direction == RIGHT
		&& bullet->GetRightSide() + speed <= static_cast<float>(bullet->GetWindowSize().x) - sideBarWidth)
	{
		return MoveRight(deltaTime);
	}

	// Self-destroy with deal damage when the edge of windows is reached
	DealDamage(GetCircleCollisionObjects(GetBulletNextPoint(bullet, deltaTime)));

	return false;
}

bool MoveLikeBulletBeh::MoveLeft(const float deltaTime) const
{
	const auto bullet = dynamic_cast<Bullet*>(_selfParent);
	if (bullet == nullptr)
	{
		return false;
	}

	if (const auto objects = IsCanMove(deltaTime); objects.empty())
	{
		bullet->MoveX(-bullet->GetSpeed() * deltaTime);

		return true;
	}
	else
	{
		DealDamage(objects);
	}

	return false;
}

bool MoveLikeBulletBeh::MoveRight(const float deltaTime) const
{
	const auto bullet = dynamic_cast<Bullet*>(_selfParent);
	if (bullet == nullptr)
	{
		return false;
	}

	if (const auto objects = IsCanMove(deltaTime); objects.empty())
	{
		bullet->MoveX(bullet->GetSpeed() * deltaTime);

		return true;
	}
	else
	{
		DealDamage(objects);
	}

	return false;
}

bool MoveLikeBulletBeh::MoveUp(const float deltaTime) const
{
	const auto bullet = dynamic_cast<Bullet*>(_selfParent);
	if (bullet == nullptr)
	{
		return false;
	}

	if (const auto objects = IsCanMove(deltaTime); objects.empty())
	{
		bullet->MoveY(-bullet->GetSpeed() * deltaTime);

		return true;
	}
	else
	{
		DealDamage(objects);
	}

	return false;
}

bool MoveLikeBulletBeh::MoveDown(const float deltaTime) const
{
	const auto bullet = dynamic_cast<Bullet*>(_selfParent);
	if (bullet == nullptr)
	{
		return false;
	}

	if (const auto objects = IsCanMove(deltaTime); objects.empty())
	{
		bullet->MoveY(bullet->GetSpeed() * deltaTime);

		return true;
	}
	else
	{
		DealDamage(objects);
	}

	return false;
}


std::vector<std::shared_ptr<BaseObj>> MoveLikeBulletBeh::GetCircleCollisionObjects(const FPoint blowCenter) const
{
	const auto* bullet = dynamic_cast<Bullet*>(_selfParent);
	if (bullet == nullptr)
	{
		return {};
	}

	std::vector<std::shared_ptr<BaseObj>> aoeCollisions{};
	constexpr int defaultCollisionReserve{5};
	aoeCollisions.reserve(defaultCollisionReserve);

	const Circle circle{.center = blowCenter, .radius = bullet->GetBulletDamageRadius()};
	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (object.get() == nullptr || _selfParent == object.get())
		{
			continue;
		}

		if (ColliderUtils::IsCollide(circle, object->GetRect()))
		{
			aoeCollisions.emplace_back(object);
		}
	}

	return aoeCollisions;
}

void MoveLikeBulletBeh::DealDamage(const std::vector<std::shared_ptr<BaseObj>>& objectList) const
//TODO: change to shared_ptr
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
			if (target && !dynamic_cast<WaterTile*>(target.get())
			    && !dynamic_cast<GrassTile*>(target.get())
			    && !dynamic_cast<IceTile*>(target.get())
			    && (target->GetIsDestructible() || thisBullet->GetTier() > 2))
			{
				target->TakeDamage(bulletDamage);
				target->SendDamageStatistics(thisBullet->GetAuthor(), thisBullet->GetFraction());
				if (const auto* otherBullet = dynamic_cast<Bullet*>(target.get()))
				{
					thisBullet->SendDamageStatistics(otherBullet->GetAuthor(), otherBullet->GetFraction());
				}
			}
		}
	}

	thisBullet->TakeDamage(bulletDamage);
}
