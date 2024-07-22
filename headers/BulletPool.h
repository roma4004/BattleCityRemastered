#pragma once

#include "pawns/Bullet.h"

#include <queue>

class BulletPool
{
	std::queue<std::shared_ptr<BaseObj>> _bullets;
	int lastId{0};
	std::shared_ptr<EventSystem> _events;
	std::string _name;
	GameMode _currentMode;
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;
	UPoint _windowSize;
	int* _windowBuffer;

public:
	BulletPool(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	           UPoint windowSize, int* windowBuffer);

	~BulletPool();
	void Subscribe();
	void Unsubscribe() const;

	std::shared_ptr<BaseObj> GetBullet(const ObjRectangle& rect, int damage, double aoeRadius, int color, int health,
	                                   Direction direction, float speed, std::string name, std::string fraction);

	void ReturnBullet(std::shared_ptr<BaseObj> bullet);
};
