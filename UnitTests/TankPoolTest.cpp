#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankPool.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/input/InputProviderForBot.h"
#include "entities/pawns/Tank.h"
#include "entities/pawns/TankResetProperty.h"
#include "enums/Author.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/TankType.h"
#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <vector>

namespace
{
//NOTE: as many tanks as TankPool pre-builds
constexpr std::size_t kSeatCount{6u};
}//namespace

class TankPoolTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<TankPool> _tankPool{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_unique<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
	}

	[[nodiscard]] std::shared_ptr<Tank> SpawnTank(const Uuid uuid, const unsigned short tier = 1u)
	{
		const double tankSize{_gameConfig.tankSize};
		const TankResetProperty property{.uuid = uuid,
										 .rect{.x = 0.0, .y = 0.0, .w = tankSize, .h = tankSize},
										 .health = _gameConfig.tankHealth,
										 .type = TankType::PLAYER1,
										 .dir = Direction::UP,
										 .tier = tier};

		return _tankPool->SpawnTank(property, std::make_unique<InputProviderForBot>(_allObjects, _gameConfig));
	}
};

//NOTE: a mode change rebuilds every spawner, so only the pool can carry a tank across it
TEST_F(TankPoolTest, AResetShelvesLiveTanksInsteadOfDroppingThem)
{
	//NOTE: weak, so a dropped tank shows as expired - its freed address could come back in the next one
	const std::weak_ptr<Tank> shelved{SpawnTank(UuidUtils::GetRandomUuid())};

	_events->EmitEvent(GameResetEvent{});

	ASSERT_FALSE(shelved.expired());

	//NOTE: the free list is a queue, so it comes back only behind the seats that were never taken
	std::vector<const Tank*> reused{};
	for (std::size_t i = 0u; i < kSeatCount; ++i)
	{
		reused.push_back(SpawnTank(UuidUtils::GetRandomUuid()).get());
	}

	EXPECT_NE(std::ranges::find(reused, shelved.lock().get()), reused.end());
}

//NOTE: Subscribe() reads the mode on activation - a tank pooled across a switch to a client must hear the host
TEST_F(TankPoolTest, AReusedTankListensUnderTheModeItSpawnsUnder)
{
	_gameConfig.gameMode = GameMode::OnePlayer;
	std::ignore = SpawnTank(UuidUtils::GetRandomUuid());

	_events->EmitEvent(GameResetEvent{});

	_gameConfig.gameMode = GameMode::PlayAsClient;
	const Uuid uuid{UuidUtils::GetRandomUuid()};
	const std::shared_ptr<Tank> reused{SpawnTank(uuid)};
	reused->Activate();

	constexpr FPoint mirrored{.x = 96.0, .y = 64.0};
	_events->EmitEvent(Key(uuid), PosChangedEvent{.pos = mirrored, .dir = Direction::DOWN, .uuid = uuid});

	EXPECT_EQ(mirrored.x, reused->GetPos().x);
	EXPECT_EQ(mirrored.y, reused->GetPos().y);
}

// one tank spawned at tier three against one walked up to it with two stars - the tier has to carry the
// stats that earned it, or a saved match comes back wrong
TEST_F(TankPoolTest, ATankSpawnedAtATierHasWhatTheStarsWouldHaveGivenIt)
{
	const auto upgraded{SpawnTank(UuidUtils::GetRandomUuid())};
	upgraded->Activate();
	_events->EmitEvent(Key(Author::Player1), BonusStarPickupEvent{});
	_events->EmitEvent(Key(Author::Player1), BonusStarPickupEvent{});

	const auto spawned{SpawnTank(UuidUtils::GetRandomUuid(), 3u)};

	EXPECT_EQ(spawned->GetTier(), upgraded->GetTier());
	EXPECT_DOUBLE_EQ(spawned->GetSpeed(), upgraded->GetSpeed());
	EXPECT_DOUBLE_EQ(spawned->GetBulletSpeed(), upgraded->GetBulletSpeed());
	EXPECT_EQ(spawned->GetBulletDamage(), upgraded->GetBulletDamage());
	EXPECT_DOUBLE_EQ(spawned->GetBulletDamageRadius(), upgraded->GetBulletDamageRadius());
}
