#pragma once

#include "Bullet.h"

#include <queue>

class BulletPool
{
	std::queue<std::shared_ptr<BaseObj>> _bullets;

public:
	std::shared_ptr<BaseObj> GetBullet(const Rectangle& rect, int damage, double aoeRadius, int color, float speed,
	                                  Direction direction, int health, int* windowBuffer, UPoint windowSize,
	                                  std::vector<std::shared_ptr<BaseObj>>* allPawns,
	                                  std::shared_ptr<EventSystem> events)
	{
		if (_bullets.empty())
		{
			return std::make_shared<Bullet>(rect, damage, aoeRadius, color, speed, direction, health, windowBuffer,
			                                windowSize, allPawns, std::move(events));
		}
		else
		{
			std::shared_ptr<BaseObj> bulletAsBase = _bullets.front();
			_bullets.pop();
			if (const auto bullet = dynamic_cast<Bullet*>(bulletAsBase.get()); bullet != nullptr)
			{
				bullet->Reset(rect, damage, aoeRadius, color, speed, direction, health);
			}

			return bulletAsBase;
		}
	}

	void ReturnBullet(std::shared_ptr<BaseObj> bullet)
	{
		if (auto bulletCast = dynamic_cast<Bullet*>(bullet.get()); bulletCast != nullptr)
		{
			bulletCast->Unsubscribe();
			_bullets.emplace(std::move(bullet));
		}
	}
};
