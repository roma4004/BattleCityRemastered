#pragma once

#include "components/EventSystem.h"
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <vector>

enum class GameMode : char8_t;
class Bullet;
class BaseObj;
class EventSystem;
class GameConfig;
struct GameResetEvent;

class BulletPool final
{
	using milliseconds = std::chrono::milliseconds;

	std::mutex _bulletsMutex{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{};
	std::queue<std::shared_ptr<BaseObj>> _bullets{};
	GameConfig& _gameConfig;
	bool _isClearing{};

	void OnGameReset(const GameResetEvent&);

public:
	BulletPool(const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
			   GameConfig& gameConfig);

	~BulletPool() = default;

	void Subscribe();

	[[nodiscard]] std::shared_ptr<Bullet> CreateNewBullet();

	[[nodiscard]] std::shared_ptr<BaseObj> SpawnBullet();

	void ReturnBullet(BaseObj* bullet);

	void Clear();

};
