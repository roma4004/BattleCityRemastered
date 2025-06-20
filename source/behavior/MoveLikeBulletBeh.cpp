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

std::vector<std::shared_ptr<BaseObj>> MoveLikeBulletBeh::IsCanMove(const float deltaTime) const
{
	const auto* bullet = dynamic_cast<Bullet*>(_selfParent);
	std::vector<std::shared_ptr<BaseObj>> aoeCollisions{};
	constexpr int defaultCollisionReserve{5};
	aoeCollisions.reserve(defaultCollisionReserve);
	if (bullet == nullptr)
	{
		return aoeCollisions;
	}

	const float speed = bullet->GetSpeed();
	const float speedX = speed * deltaTime;
	const float speedY = speed * deltaTime;
	ObjRectangle bulletNextPosRect;
	if (const Direction dir = bullet->GetDirection();
		dir == UP)
	{
		bulletNextPosRect = ObjRectangle{
				.x = bullet->GetX(),
				.y = bullet->GetY() - speedY,
				.w = bullet->GetWidth(),
				.h = bullet->GetHeight() + speedY};
	}
	else if (dir == DOWN)
	{
		bulletNextPosRect = ObjRectangle{
				.x = bullet->GetX(),
				.y = bullet->GetY(),
				.w = bullet->GetWidth(),
				.h = bullet->GetHeight() + speedY};
	}
	else if (dir == LEFT)
	{
		bulletNextPosRect = ObjRectangle{
				.x = bullet->GetX() - speedX,//TODO: write bullet test that can damage tank from all sides
				.y = bullet->GetY(),
				.w = bullet->GetWidth() + speedX,
				.h = bullet->GetHeight()};
	}
	else if (dir == RIGHT)
	{
		bulletNextPosRect = ObjRectangle{
				.x = bullet->GetX(),
				.y = bullet->GetY(),
				.w = bullet->GetWidth() + speedX,
				.h = bullet->GetHeight()};
	}

	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (object.get() == nullptr || bullet == object.get())
		{
			continue;
		}

		if (ColliderUtils::IsCollide(bulletNextPosRect, object->GetRect()))
		{
			if (!object->GetIsPenetrable())
			{
				CheckCircleAoE(FPoint{.x = bullet->GetX() + speedX, .y = bullet->GetY() + speedY}, aoeCollisions);
				return aoeCollisions;
			}
		}
	}

	return aoeCollisions;
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
	if (constexpr int sideBarWidth = 175;
		direction == RIGHT
		&& bullet->GetRightSide() + speed <= static_cast<float>(bullet->GetWindowSize().x) - sideBarWidth)
	{
		return MoveRight(deltaTime);
	}

	bullet->SetIsAlive(false);// Self-destroy when edge of windows is reached

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


void MoveLikeBulletBeh::CheckCircleAoE(const FPoint blowCenter, std::vector<std::shared_ptr<BaseObj>>& aoeList) const
{
	const auto* bullet = dynamic_cast<Bullet*>(_selfParent);
	if (bullet == nullptr)
	{
		return;
	}

	const Circle circle{.center = blowCenter, .radius = bullet->GetBulletDamageRadius()};
	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (object.get() == nullptr || _selfParent == object.get())
		{
			continue;
		}

		if (ColliderUtils::IsCollide(circle, object->GetRect()))
		{
			aoeList.emplace_back(object);
		}
	}
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
