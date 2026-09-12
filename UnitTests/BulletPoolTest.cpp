#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/BaseObj.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/InputChannel.h"
#include "enums/TextureType.h"
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
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	double _tankSize{};
	int _tankHealth{100};
	Uuid _uuid{};
	EventSubscription _spawnQueueSub{};
	EventSubscription _disposalSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		//NOTE: the pool goes first on purpose - it used to listen on PostTickUpdate alongside the
		//sweep, and then this order alone decided whether a free slot was still in the world
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_disposalSub = TestUtils::WireWorldDisposal(_events, _allObjects);
		_tankSize = _gameConfig.tankSize;

		_allObjects.reserve(64u);
	}
};

// More shots than the pool pre-generates, then all of them die: the very same objects come back
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

// A bullet that went back to the pool is off the bus again, so it neither draws nor ticks there
TEST_F(BulletPoolTest, ReturnedBulletLeavesTheBus)
{
	int bulletDraws{0};
	const EventSubscription drawSub = _events->AddListener([&bulletDraws](const DrawObjEvent& event)
	{
		if (event.texture == TextureType::Bullet)
		{
			++bulletDraws;
		}
	});

	std::shared_ptr<Tank> player = TestUtils::CreatePlayer(
			ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize}, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const std::shared_ptr<BaseObj> bullet = _allObjects.back();
	ASSERT_NE(nullptr, dynamic_cast<Bullet*>(bullet.get()));

	_events->EmitEvent(DrawEvent{});
	EXPECT_EQ(1, bulletDraws);

	bullet->SetIsAlive(false);
	_events->EmitEvent(PostTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	_events->EmitEvent(DrawEvent{});
	EXPECT_EQ(1, bulletDraws);
}

// A slot is free only once the world has let go of the bullet: handed back any earlier, it sits in
// the free list and in _allObjects at the same time, and the next shot reuses an object still there
TEST_F(BulletPoolTest, ASlotComesBackOnlyAfterTheWorldLetGo)
{
	const std::shared_ptr<Bullet> bullet = _bulletPool->SpawnBullet({});
	_allObjects.emplace_back(bullet);

	bool wasStillInTheWorld{true};
	const EventSubscription despawnSub = _events->AddListener(
			[this, &wasStillInTheWorld, raw = bullet.get()](const DespawnedEvent&)
	{
		wasStillInTheWorld = std::ranges::any_of(_allObjects, [raw](const std::shared_ptr<BaseObj>& obj)
		{
			return obj.get() == raw;
		});
	});

	bullet->SetIsAlive(false);
	_events->EmitEvent(PostTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(wasStillInTheWorld);
}
