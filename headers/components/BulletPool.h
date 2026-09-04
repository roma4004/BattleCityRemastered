#pragma once

#include "components/EventSystem.h"
#include "components/PooledSlots.h"
#include "entities/pawns/BulletResetProperty.h"
#include "utils/Uuid.h"
#include <chrono>
#include <memory>
#include <optional>
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

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	const std::vector<std::shared_ptr<BaseObj>>& _allObjects;
	PooledSlots<Bullet> _slots{};
	const GameConfig& _gameConfig;

	void OnGameReset(const GameResetEvent&);
	void OnPostTickUpdate(const PostTickUpdateEvent&);

	[[nodiscard]] std::shared_ptr<Bullet> CreateNewBullet() const;

	void Subscribe();

public:
	BulletPool(const std::shared_ptr<EventSystem>& events, const std::vector<std::shared_ptr<BaseObj>>& allObjects,
			   const GameConfig& gameConfig);

	~BulletPool() = default;

	//NOTE: armed here - the pool owns both the free list and Bullet::Reset
	[[nodiscard]] std::shared_ptr<Bullet> SpawnBullet(const BulletResetProperty& property,
													 const std::optional<Uuid>& uuid = std::nullopt);
};
