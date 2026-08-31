#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/BaseObj.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/Faction.h"
#include "enums/GameMode.h"
#include "enums/InputChannel.h"
#include "enums/TextureType.h"
#include "gtest/gtest.h"
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
	double _tankSpeed{142.0};
	int _tankHealth{100};
	Uuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};
	EventSubscription _disposalSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_disposalSub = TestUtils::WireWorldDisposal(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
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
		inFlight.push_back(_bulletPool->SpawnBullet(std::nullopt));
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
		inFlight.push_back(_bulletPool->SpawnBullet(std::nullopt));
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

	std::shared_ptr<Player> player = TestUtils::CreateTank<Player>(
			ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize}, _tankHealth, _uuid, "Player1",
			Faction::PlayerTeam, _allObjects, _events, 1u, _tankSpeed, Direction::DOWN, _gameMode, _bulletPool,
			_gameConfig);
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
