#pragma once

#include "../../headers/Point.h"
#include <memory>
#include <mutex>
#include <queue>
#include <string>

enum GameMode : char8_t;
enum Direction : char8_t;
struct ObjRectangle;
struct SDL_Renderer;
class Bullet;
class BaseObj;
class EventSystem;
// class BaseObj;

class BulletPool final
{
	using milliseconds = std::chrono::milliseconds;

	std::mutex _bulletsMutex;
	std::string _name;
	GameMode _gameMode;
	UPoint _windowSize;
	bool _isClearing{false};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;
	std::queue<std::shared_ptr<BaseObj>> _bullets;

public:
	BulletPool(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	           UPoint windowSize, GameMode gameMode);

	~BulletPool();

	void Subscribe();
	void Unsubscribe() const;

	std::shared_ptr<Bullet> CreateNewBullet();

	std::shared_ptr<BaseObj> SpawnBullet();

	void ReturnBullet(BaseObj* bullet);

	void Clear();

	static std::string GetCurrentTimeString();

};
