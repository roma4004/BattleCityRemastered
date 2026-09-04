#pragma once

#include "components/EventSystem.h"
#include "components/PooledSlots.h"
#include <memory>
#include <vector>

struct GameResetEvent;
struct PostTickUpdateEvent;
struct TankResetProperty;
class BaseObj;
class BulletPool;
class EventSystem;
class GameConfig;
class IInputProvider;
class Tank;

//NOTE: the same arrangement BulletPool has - the pool owns its tanks for the whole match, _allObjects
//only borrows them. A seat is not a type any more, so any free tank fits any seat.
class TankPool final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	const std::vector<std::shared_ptr<BaseObj>>& _allObjects;
	const GameConfig& _gameConfig;
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	PooledSlots<Tank> _slots{};

	void OnGameReset(const GameResetEvent&);
	void OnPostTickUpdate(const PostTickUpdateEvent&);

	[[nodiscard]] std::shared_ptr<Tank> CreateNewTank() const;

	void Subscribe();

public:
	TankPool(const std::shared_ptr<EventSystem>& events, const std::vector<std::shared_ptr<BaseObj>>& allObjects,
			 const GameConfig& gameConfig, const std::shared_ptr<BulletPool>& bulletPool);

	~TankPool();

	//NOTE: the driver comes in with the seat - taking a tank out of the pool is swapping who steers it
	[[nodiscard]] std::shared_ptr<Tank> SpawnTank(const TankResetProperty& property,
												  std::unique_ptr<IInputProvider> driver);
};
