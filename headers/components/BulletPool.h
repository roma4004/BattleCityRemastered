#pragma once

#include "components/EventSystem.h"
#include <chrono>
#include <memory>
#include <mutex>
#include <queue>
#include <vector>

enum class GameMode : char8_t;
class Bullet;
class BaseObj;
class EventSystem;
class GameConfig;
struct GameResetEvent;
struct PostTickUpdateEvent;

class BulletPool final
{
	using milliseconds = std::chrono::milliseconds;

	std::mutex _bulletsMutex{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	const std::vector<std::shared_ptr<BaseObj>>& _allObjects;
	std::queue<std::shared_ptr<Bullet>> _free{};
	std::vector<std::shared_ptr<Bullet>> _inFlight{};
	const GameConfig& _gameConfig;

	void OnGameReset(const GameResetEvent&);
	void OnPostTickUpdate(const PostTickUpdateEvent&);

	[[nodiscard]] std::shared_ptr<Bullet> CreateNewBullet() const;

public:
	BulletPool(const std::shared_ptr<EventSystem>& events, const std::vector<std::shared_ptr<BaseObj>>& allObjects,
			   const GameConfig& gameConfig);

	~BulletPool() = default;

	void Subscribe();

	[[nodiscard]] std::shared_ptr<BaseObj> SpawnBullet();

	void Clear();
};
