#pragma once

#include "pawns/Bullet.h"

#include <queue>

class BulletPool
{
	std::queue<std::shared_ptr<BaseObj>> _bullets;

public:
	std::shared_ptr<BaseObj> GetBullet(const Rectangle& rect, int damage, double aoeRadius, int color, int health,
	                                   int* windowBuffer, UPoint windowSize, Direction direction, float speed,
	                                   std::vector<std::shared_ptr<BaseObj>>* allObjects,
	                                   const std::shared_ptr<EventSystem>& events, std::string name,
	                                   std::string fraction);

	void ReturnBullet(std::shared_ptr<BaseObj> bullet);
};
