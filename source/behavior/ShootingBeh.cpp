#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/Point.h"
#include "../../headers/components/BulletPool.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/pawns/Tank.h"

#include <functional>
#include <memory>

ShootingBeh::ShootingBeh(BaseObj* selfParent, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                         std::shared_ptr<EventSystem> events, std::shared_ptr<BulletPool> bulletPool)
	: _selfParent{selfParent},
	  _allObjects{allObjects},
	  _events{std::move(events)},
	  _bulletPool{std::move(bulletPool)} {}

ShootingBeh::~ShootingBeh() = default;

// inline float Distance(const FPoint a, const FPoint b)
// {
// 	return static_cast<float>(std::sqrt(std::pow(b.x - a.x, 2) + std::pow(b.y - a.y, 2)));
// }

float ShootingBeh::FindMinDistance(const std::vector<std::weak_ptr<BaseObj>>& objects,
                                   const std::function<float(const std::shared_ptr<BaseObj>&)>& sideDiff) const
{
	const auto* tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return 0.f;
	}

	float minDist = static_cast<float>(tank->GetWindowSize().x * tank->GetWindowSize().y);
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

//Note: {-1.f, -1.f} this is try shooting outside screen
ObjRectangle ShootingBeh::GetBulletStartRect() const
{
	const auto* tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return {};
	}

	const FPoint tankHalf = {.x = tank->GetWidth() / 2.f, .y = tank->GetHeight() / 2.f};
	const FPoint tankPos = tank->GetPos();
	const float tankRightX = tank->GetRightSide();
	const float tankBottomY = tank->GetBottomSide();
	const FPoint tankCenter = {.x = tankPos.x + tankHalf.x, .y = tankPos.y + tankHalf.y};

	const float bulletWidth = tank->GetBulletWidth();
	const float bulletHeight = tank->GetBulletHeight();
	const FPoint bulletHalf = {.x = bulletWidth / 2.f, .y = bulletHeight / 2.f};
	ObjRectangle bulletRect = {.x = -1, .y = -1, .w = bulletWidth, .h = bulletHeight};

	if (const Direction dir = tank->GetDirection();
		dir == UP && tankPos.y - bulletHalf.y >= 0.f)//TODO: rewrite check with zero to use epsilon
	{
		bulletRect.x = tankCenter.x - bulletHalf.x;
		bulletRect.y = tankPos.y - bulletHalf.y;
	}
	else if (dir == DOWN && tankBottomY + bulletHalf.y <= static_cast<float>(tank->GetWindowSize().y))
	{
		bulletRect.x = tankCenter.x - bulletHalf.x;
		bulletRect.y = tankBottomY - bulletHalf.y;
	}
	else if (dir == LEFT && tankPos.x - bulletWidth >= 0.f)//TODO: rewrite check with zero to use epsilon
	{
		bulletRect.x = tankPos.x - bulletHalf.x;
		bulletRect.y = tankCenter.y - bulletHalf.y;
	}
	else if (dir == RIGHT && tankRightX + bulletHalf.x + bulletWidth <= static_cast<float>(tank->GetWindowSize().x))
	{
		bulletRect.x = tankRightX - bulletHalf.x;
		bulletRect.y = tankCenter.y - bulletHalf.y;
	}

	return bulletRect;
}

void ShootingBeh::Shot() const
{
	const auto* tank = dynamic_cast<Tank*>(_selfParent);
	if (tank == nullptr)
	{
		return;
	}

	ObjRectangle rect = GetBulletStartRect();
	if (rect.x < 0.f || rect.y < 0.f)
	{
		//Try shooting outside screen
		return;
	}

	constexpr int color = 0xffffff;
	const int damage = tank->GetBulletDamage();
	const double aoeRadius = tank->GetBulletDamageRadius();
	constexpr int health = 1;
	const Direction dir = tank->GetDirection();
	const float speed = tank->GetBulletSpeed();
	std::string author = tank->GetName();
	std::string fraction = tank->GetFraction();
	const int tier = tank->GetTier();
	_bulletPool->SpawnBullet(
			std::move(rect), damage, aoeRadius, color, health, dir, speed, std::move(author), std::move(fraction),
			tier);
}
