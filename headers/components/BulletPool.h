#pragma once

#include "Point.h"
#include <memory>
#include <mutex>
#include <queue>
#include <string>

enum class GameMode : char8_t;
enum class Direction : char8_t;
struct ObjRectangle;
struct SDL_Renderer;
class Bullet;
class BaseObj;
class EventSystem;

class BulletPool final
{
	using milliseconds = std::chrono::milliseconds;

	std::mutex _bulletsMutex{};
	std::string _name{};
	UPoint _windowSize{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{};
	std::queue<std::shared_ptr<BaseObj>> _bullets{};
	GameMode _gameMode{};
	bool _isClearing{false};

public:
	BulletPool(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	           UPoint windowSize, GameMode gameMode);

	~BulletPool();

	void Subscribe();
	void Unsubscribe() const;

	[[nodiscard]] std::shared_ptr<Bullet> CreateNewBullet();

	[[nodiscard]] std::shared_ptr<BaseObj> SpawnBullet();

	void ReturnBullet(BaseObj* bullet);

	void Clear();

	[[nodiscard]] static std::string GetCurrentTimeString();

};
