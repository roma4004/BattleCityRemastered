#include "../headers/BulletPool.h"
#include "../headers/pawns/Bullet.h"

std::shared_ptr<BaseObj> BulletPool::GetBullet(const Rectangle& rect, int damage, double aoeRadius, int color,
                                               int health, int* windowBuffer, UPoint windowSize, Direction direction,
                                               float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                                               std::shared_ptr<EventSystem> events, std::string name,
                                               std::string fraction)
{
	if (_bullets.empty())
	{
		return std::make_shared<Bullet>(rect, damage, aoeRadius, color, health, windowBuffer, windowSize, direction,
		                                speed, allObjects, std::move(events), std::move(name), std::move(fraction));
	}

	std::shared_ptr<BaseObj> bulletAsBase = _bullets.front();
	_bullets.pop();
	if (const auto bullet = dynamic_cast<Bullet*>(bulletAsBase.get()); bullet != nullptr)
	{
		bullet->Reset(rect, damage, aoeRadius, color, speed, direction, health, std::move(name), std::move(fraction));
	}

	return bulletAsBase;
}

void BulletPool::ReturnBullet(std::shared_ptr<BaseObj> bullet)
{
	if (const auto bulletCast = dynamic_cast<Bullet*>(bullet.get()); bulletCast != nullptr)
	{
		bulletCast->Disable();
		_bullets.emplace(std::move(bullet));
	}
}
