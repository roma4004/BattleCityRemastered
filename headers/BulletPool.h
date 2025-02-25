#pragma once

#include "GameMode.h"
#include "Point.h"

#include <memory>
#include <queue>
#include <string>

enum GameMode : char8_t;
enum Direction : char8_t;
struct ObjRectangle;
class Bullet;
class BaseObj;
class EventSystem;

class BulletPool final
{
	std::queue<std::shared_ptr<BaseObj>> _bullets;
	int lastId{0};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::string _name;
	GameMode _currentMode;
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;
	UPoint _windowSize;
	std::shared_ptr<int[]> _windowBuffer{nullptr};

public:
	BulletPool(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	           UPoint windowSize, std::shared_ptr<int[]> windowBuffer, GameMode currentGameMode = GameMode::Demo);

	~BulletPool();

	void Subscribe();
	void Unsubscribe() const;

	std::shared_ptr<BaseObj> GetBullet(const ObjRectangle& rect, int damage, double aoeRadius, int color, int health,
	                                   Direction direction, float speed, std::string author, std::string fraction);

	void ReturnBullet(std::shared_ptr<BaseObj> bullet);

	void Clear();
};
