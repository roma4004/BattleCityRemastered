#pragma once

#include <memory>
#include <mutex>
#include <queue>
#include <string>

enum class GameMode : char8_t;
class Bullet;
class BaseObj;
class EventSystem;
class GameConfig;

class BulletPool final
{
	using milliseconds = std::chrono::milliseconds;

	std::mutex _bulletsMutex{};
	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{};
	std::queue<std::shared_ptr<BaseObj>> _bullets{};
	GameMode _gameMode{};
	GameConfig& _gameConfig;
	bool _isClearing{};

public:
	BulletPool(const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
			   GameConfig& gameConfig);

	~BulletPool();

	void Subscribe();
	void Unsubscribe() const;

	[[nodiscard]] std::shared_ptr<Bullet> CreateNewBullet();

	[[nodiscard]] std::shared_ptr<BaseObj> SpawnBullet();

	void ReturnBullet(BaseObj* bullet);

	void Clear();

	[[nodiscard]] static std::string GetCurrentTimeString();
};
