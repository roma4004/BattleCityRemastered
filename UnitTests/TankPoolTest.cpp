#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankPool.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/input/InputProviderForBot.h"
#include "entities/pawns/Tank.h"
#include "entities/pawns/TankResetProperty.h"
#include "enums/Author.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
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
//NOTE: four enemy seats and two player ones, the same count TankPool pre-builds
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

	[[nodiscard]] std::shared_ptr<Tank> SpawnTank(const Uuid uuid)
	{
		const double tankSize{_gameConfig.tankSize};
		const TankResetProperty property{.uuid = uuid,
										 .rect{.x = 0.0, .y = 0.0, .w = tankSize, .h = tankSize},
										 .health = _gameConfig.tankHealth,
										 .speed = _gameConfig.tankSpeed,
										 .author = Author::Player1,
										 .dir = Direction::UP};

		return _tankPool->SpawnTank(property, std::make_unique<InputProviderForBot>(_allObjects, _gameConfig));
	}
};

//NOTE: a mode change ends the match and rebuilds every spawner, so the pool is the only thing that
//can carry a tank across it
TEST_F(TankPoolTest, AResetShelvesLiveTanksInsteadOfDroppingThem)
{
	//NOTE: weak, not a raw pointer - a dropped tank frees an address the next one can be handed back
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

//NOTE: the mode is a copy inside the pawn, and Subscribe() branches on it - a tank that sat in the
//pool through a switch to a client would otherwise never listen for the host's updates
TEST_F(TankPoolTest, AReusedTankListensUnderTheModeItSpawnsUnder)
{
	_gameConfig.gameMode = GameMode::OnePlayer;
	std::ignore = SpawnTank(UuidUtils::GetRandomUuid());

	_events->EmitEvent(GameResetEvent{});

	_gameConfig.gameMode = GameMode::PlayAsClient;
	const Uuid uuid{UuidUtils::GetRandomUuid()};
	const std::shared_ptr<Tank> reused = SpawnTank(uuid);
	reused->Activate();

	constexpr FPoint mirrored{.x = 96.0, .y = 64.0};
	_events->EmitEvent(Key(uuid), PosChangedEvent{.pos = mirrored, .dir = Direction::DOWN, .uuid = uuid});

	EXPECT_EQ(mirrored.x, reused->GetPos().x);
	EXPECT_EQ(mirrored.y, reused->GetPos().y);
}
