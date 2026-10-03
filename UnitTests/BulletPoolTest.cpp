#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/BaseObj.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/InputChannel.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <memory>
#include <set>
#include <vector>

class BulletPoolTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	double _tankSize{};
	int _tankHealth{100};
	EventSubscription _spawnQueueSub{};
	EventSubscription _disposalSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		//NOTE: the pool before the sweep on purpose - reclaiming on PostTickUpdate it would run first and get caught
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_disposalSub = TestUtils::WireWorldDisposal(_events, _allObjects);
		_tankSize = _gameConfig.tankSize;

		_allObjects.reserve(64u);
	}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Direction dir)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _tankHealth, Author::Player1, _allObjects, _events, dir,
											_tankPool, _gameConfig)};

		return player;
	}
};

// take 25 bullets from a pool that pre-generates 20, kill them all, sweep, take 25 again: the second
// round is the same 25 objects
TEST_F(BulletPoolTest, SpentBulletsAreHandedOutAgain)
{
	constexpr size_t shots{25u};

	std::vector<std::shared_ptr<BaseObj>> inFlight{};
	std::set<const BaseObj*> firstRound{};
	for (size_t i = 0u; i < shots; ++i)
	{
		inFlight.push_back(_bulletPool->SpawnBullet({}));
		firstRound.insert(inFlight.back().get());
	}

	//NOTE: nothing is shared while in flight - the pool grew past its 20 pre-generated bullets
	EXPECT_EQ(shots, firstRound.size());

	for (const std::shared_ptr<BaseObj>& bullet: inFlight)
	{
		bullet->SetIsAlive(false);
	}

	//NOTE: dropping every outside reference must not destroy anything - the pool owns them
	inFlight.clear();
	_events->EmitEvent(PostTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	std::set<const BaseObj*> secondRound{};
	for (size_t i = 0u; i < shots; ++i)
	{
		inFlight.push_back(_bulletPool->SpawnBullet({}));
		secondRound.insert(inFlight.back().get());
	}

	EXPECT_EQ(firstRound, secondRound);
}

// a player fires and the bullet flies; kill it and sweep, and it is off the bus - brought back to life by
// hand, it is still left where it stopped, not merely hidden
TEST_F(BulletPoolTest, ReturnedBulletLeavesTheBus)
{
	CreatePlayer({.x = 0.0, .y = 0.0}, Direction::DOWN);

	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = false});

	const std::shared_ptr<BaseObj> bullet{_allObjects.back()};
	ASSERT_NE(nullptr, dynamic_cast<Bullet*>(bullet.get()));

	bullet->SetIsAlive(false);
	_events->EmitEvent(PostTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	bullet->SetIsAlive(true);
	const FPoint parked{bullet->GetPos()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(parked, bullet->GetPos());
}

// Handed back any earlier, a bullet sits in the free list and in _allObjects at once, and the next shot reuses it
TEST_F(BulletPoolTest, ASlotComesBackOnlyAfterTheWorldLetGo)
{
	const std::shared_ptr<Bullet> bullet{_bulletPool->SpawnBullet({})};
	_allObjects.emplace_back(bullet);

	bool wasStillInTheWorld{true};
	const EventSubscription despawnSub{_events->AddListener(
			[this, &wasStillInTheWorld, raw = bullet.get()](const DespawnedEvent&)
	{
		wasStillInTheWorld = std::ranges::any_of(_allObjects, [raw](const std::shared_ptr<BaseObj>& obj)
		{
			return obj.get() == raw;
		});
	})};

	bullet->SetIsAlive(false);
	_events->EmitEvent(PostTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(wasStillInTheWorld);
}
