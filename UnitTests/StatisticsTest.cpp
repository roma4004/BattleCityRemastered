#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/managers/BonusManager.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "components/GameStatistics.h"
#include "components/StatisticsData.h"
#include "entities/bonuses/Bonus.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Author.h"
#include "enums/Direction.h"
#include "enums/InputChannel.h"
#include "enums/ObstacleType.h"
#include "geometry/Point.h"
#include "enums/PlayerSlot.h"
#include "gtest/gtest.h"
#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <string_view>
#include <utility>

// every case: a target, a bullet a step short of it, one tick, the counter that pair owns - one hit is the kill
class StatisticsTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<BonusManager> _bonusManager{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	BulletCaliber _caliber{
			.speed = 300.0, .damage = 1u, .health = 1, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	double _tankSize{};
	Uuid _uuid{};
	unsigned short _tankHealth{1u};
	unsigned short _bulletHealth{1u};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_bonusManager = std::make_shared<BonusManager>(_events, _gameConfig);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_statistics = std::make_shared<GameStatistics>(_events);
		_tankSize = _gameConfig.tankSize;

		_allObjects.reserve(5);
	}

	void TearDown() override {}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author = Author::Player1,
									   const Direction dir = Direction::UP)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool,
											_gameConfig)};

		return player;
	}

	std::shared_ptr<Tank> CreateBot(const FPoint pos, const Author author, const Direction dir,
									const unsigned short tier = 1u)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto bot{TestUtils::CreateBot(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool, _gameConfig,
									  tier)};

		return bot;
	}

	std::shared_ptr<Bullet> CreateBullet(const FPoint pos, const Direction dir, const Author author)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _caliber.size.x, .h = _caliber.size.y};
		auto bullet{TestUtils::CreateBullet(rect, _bulletHealth, _bulletPool, _events, _caliber, dir,
											author)};

		return bullet;
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const FPoint pos, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, pos, type, _gameConfig);
	}
};

// a player shot by the enemy team: the hit stays with the seat, the kill goes to the team's shared counter
TEST_F(StatisticsTest, APlayerShotByTheEnemyTeam)
{
	constexpr std::array kSeats{PlayerSlot::P1, PlayerSlot::P2};
	for (const PlayerSlot seat: kSeats)
	{
		SCOPED_TRACE(ToString(seat));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		StatisticsData expected{_statistics->GetData()};
		++expected.seats[SeatIndex(seat)].hitByEnemyTeam;
		++expected.enemyTeam.playerKills;
		CreatePlayer({.x = 0.0, .y = 0.0}, AuthorOf(seat));
		CreateBullet({.x = _tankSize / 2.0, .y = _tankSize}, Direction::UP, Author::Enemy1);

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_statistics->GetData(), expected);
	}
}

// shot by the other seat it is friendly fire, the hit and the kill both kept by the seat it happened to
TEST_F(StatisticsTest, APlayerShotByTheOtherSeat)
{
	constexpr std::array kShooters{std::pair{PlayerSlot::P1, Author::Player2},
								   std::pair{PlayerSlot::P2, Author::Player1}};
	for (const auto& [seat, shooter]: kShooters)
	{
		SCOPED_TRACE(ToString(seat));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		StatisticsData expected{_statistics->GetData()};
		++expected.seats[SeatIndex(seat)].friendlyHitsTaken;
		++expected.seats[SeatIndex(seat)].friendlyKillsTaken;
		CreatePlayer({.x = 0.0, .y = 0.0}, AuthorOf(seat));
		CreateBullet({.x = _tankSize / 2.0, .y = _tankSize + 1.0}, Direction::UP, shooter);

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_statistics->GetData(), expected);
	}
}

// a bot shot by a bot - the enemy team counts its own friendly fire too, hit and kill alike
TEST_F(StatisticsTest, AnEnemyShotByItsOwnTeam)
{
	CreateBot({.x = _tankSize * 2.0 + 2.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
	CreateBullet({.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0, .y = _tankSize}, Direction::UP, Author::Enemy2);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyTeam.friendlyHitsTaken, 1u);
	EXPECT_EQ(_statistics->GetData().enemyTeam.friendlyKillsTaken, 1u);
}

// a bot shot by a seat is that seat's hit and kill - the seats never share a bucket
TEST_F(StatisticsTest, AnEnemyShotByASeat)
{
	constexpr std::array kSeats{PlayerSlot::P1, PlayerSlot::P2};
	for (const PlayerSlot seat: kSeats)
	{
		SCOPED_TRACE(ToString(seat));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		StatisticsData expected{_statistics->GetData()};
		++expected.seats[SeatIndex(seat)].enemyHits;
		++expected.seats[SeatIndex(seat)].enemyKills;
		CreateBot({.x = _tankSize * 2.0 + 2.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
		CreateBullet({.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0, .y = _tankSize}, Direction::UP, AuthorOf(seat));

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_statistics->GetData(), expected);
	}
}

// a third seat scores into its own column
TEST_F(StatisticsTest, AThirdSeatScoresIntoItsOwnColumn)
{
	CreateBot({.x = _tankSize * 2.0 + 2.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
	CreateBullet({.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0, .y = _tankSize}, Direction::UP, Author::Player3);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().Seat(PlayerSlot::P3).enemyHits, 1u);
	EXPECT_EQ(_statistics->GetData().Seat(PlayerSlot::P1).enemyHits, 0u);
}

// the counter is about bullets touching bullets - touching a wall, broken or not, or the field's edge is no bullet hit
TEST_F(StatisticsTest, ABulletTouchingAnythingButABulletIsNoBulletHit)
{
	struct Touch final
	{
		std::string_view why;
		std::optional<ObstacleType> wall;
		Direction dir{};
	};

	constexpr std::array kTouches{
			Touch{.why = "a brick it brings down", .wall = ObstacleType::Brick, .dir = Direction::DOWN},
			Touch{.why = "steel that swallows it", .wall = ObstacleType::Steel, .dir = Direction::DOWN},
			Touch{.why = "the edge of the field", .wall = std::nullopt, .dir = Direction::UP}};
	for (const Touch& touch: kTouches)
	{
		SCOPED_TRACE(touch.why);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		if (touch.wall.has_value())
		{
			const auto wall{SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize + _caliber.size.y + 1}, *touch.wall)};
			_caliber.damage = wall->GetHealth();
		}
		CreateBullet({.x = 0.0, .y = touch.dir == Direction::UP ? 0.0 : _tankSize}, touch.dir, Author::Player1);

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_statistics->GetData().Seat(PlayerSlot::P1).bulletHits, 0u);
	}
}

// a wall a shot brings down is counted to whoever fired it, brick and steel apart - steel gives only to a tier 3 shot
TEST_F(StatisticsTest, AWallBroughtDownIsCountedToItsShooter)
{
	struct Shot final
	{
		ObstacleType wall;
		Author author;
		void (*credit)(StatisticsData&);
	};

	constexpr std::array kShots{
			 Shot{.wall = ObstacleType::Brick,
				  .author = Author::Enemy1,
				  .credit = [](StatisticsData& data) { ++data.enemyTeam.brickWallKills; }},
			 Shot{.wall = ObstacleType::Brick,
				  .author = Author::Player1,
				  .credit = [](StatisticsData& data) { ++data.seats[SeatIndex(PlayerSlot::P1)].brickWallKills; }},
			 Shot{.wall = ObstacleType::Brick,
				  .author = Author::Player2,
				  .credit = [](StatisticsData& data) { ++data.seats[SeatIndex(PlayerSlot::P2)].brickWallKills; }},
			 Shot{.wall = ObstacleType::Steel,
				  .author = Author::Enemy1,
				  .credit = [](StatisticsData& data) { ++data.enemyTeam.steelWallKills; }},
			 Shot{.wall = ObstacleType::Steel,
				  .author = Author::Player1,
				  .credit = [](StatisticsData& data) { ++data.seats[SeatIndex(PlayerSlot::P1)].steelWallKills; }},
			 Shot{.wall = ObstacleType::Steel,
				  .author = Author::Player2,
				  .credit = [](StatisticsData& data) { ++data.seats[SeatIndex(PlayerSlot::P2)].steelWallKills; }}};
	for (const auto& [type, author, credit]: kShots)
	{
		SCOPED_TRACE(testing::Message() << static_cast<int>(type) << ' ' << static_cast<int>(author));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		StatisticsData expected{_statistics->GetData()};
		credit(expected);
		const auto wall{SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize + _caliber.size.y + 1}, type)};
		_caliber.damage = wall->GetHealth();
		_caliber.tier = type == ObstacleType::Steel ? 3u : 1u;
		CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, author);

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_statistics->GetData(), expected);
	}
}

// two shots meeting head-on credit both authors, each in their own counter - two enemy shots fill one bucket twice
TEST_F(StatisticsTest, BulletsMeetingCreditBothAuthors)
{
	struct Meeting final
	{
		Author down;
		Author up;
		void (*credit)(StatisticsData&);
	};

	constexpr std::array kMeetings{
			 Meeting{.down = Author::Enemy1,
					 .up = Author::Enemy2,
					 .credit = [](StatisticsData& data)
					 {
						 data.enemyTeam.bulletHits += 2u;
					 }},
			 Meeting{.down = Author::Player1,
					 .up = Author::Player2,
					 .credit = [](StatisticsData& data)
					 {
						 ++data.seats[SeatIndex(PlayerSlot::P1)].bulletHits;
						 ++data.seats[SeatIndex(PlayerSlot::P2)].bulletHits;
					 }},
			 Meeting{.down = Author::Player1,
					 .up = Author::Enemy1,
					 .credit = [](StatisticsData& data)
					 {
						 ++data.seats[SeatIndex(PlayerSlot::P1)].bulletHits;
						 ++data.enemyTeam.bulletHits;
					 }},
			 Meeting{.down = Author::Player2,
					 .up = Author::Enemy1,
					 .credit = [](StatisticsData& data)
					 {
						 ++data.seats[SeatIndex(PlayerSlot::P2)].bulletHits;
						 ++data.enemyTeam.bulletHits;
					 }}};
	for (const auto& [down, up, credit]: kMeetings)
	{
		SCOPED_TRACE(testing::Message() << static_cast<int>(down) << ' ' << static_cast<int>(up));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		StatisticsData expected{_statistics->GetData()};
		credit(expected);
		CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, down);
		CreateBullet({.x = 0.0, .y = _tankSize + _caliber.size.y + 1.0}, Direction::UP, up);

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_statistics->GetData(), expected);
	}
}

// a pickup is counted to whoever took it - a bot to the enemy team's shared column, a player to that seat's own
TEST_F(StatisticsTest, ABonusPickupIsCountedToItsTaker)
{
	const std::array takers{std::pair{Author::Enemy1, 0.0}, std::pair{Author::Player1, (_tankSize + 1.0) * 2.0},
							std::pair{Author::Player2, (_tankSize + 1.0) * 4.0}};
	for (const auto& [author, x]: takers)
	{
		SCOPED_TRACE(static_cast<int>(author));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		StatisticsData expected{_statistics->GetData()};
		const std::optional<PlayerSlot> seat{SlotOf(author)};
		++(seat.has_value() ? expected.seats[SeatIndex(*seat)].bonusPickups : expected.enemyTeam.bonusPickups);
		if (seat.has_value())
		{
			CreatePlayer({.x = x, .y = 0.0}, author, Direction::DOWN);
			_events->EmitEvent(Key(LocalInput(*seat)), MoveDownEvent{.isPressed = true});
		}
		else
		{
			CreateBot({.x = x, .y = 0.0}, author, Direction::DOWN);
		}
		_bonusSpawner->SpawnRandomBonus({.x = x, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_statistics->GetData(), expected);
	}
}

// a bonus nobody touched: the lifetime runs out and the expiry is counted with no author
TEST_F(StatisticsTest, BonusExpiredCountedWithNoAuthor)
{
	//NOTE: how long a bonus lives is the spawner's, so the test shortens it instead of building one by hand
	_gameConfig.bonusLifeTimeCooldown = std::chrono::milliseconds{};
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);
	ASSERT_EQ(_allObjects.size(), 1u);

	EXPECT_EQ(_statistics->GetData().bonusExpired, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusExpired, 1u);
	EXPECT_FALSE(_allObjects.back()->GetIsAlive());

	//NOTE: the timer is one-shot - a second tick must not keep counting the same bonus
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusExpired, 1u);
}

// shot and picked up are two different buckets, so one bonus is destroyed and the next one taken
TEST_F(StatisticsTest, BonusShotIsCountedAndPickupIsNot)
{
	const ObjRectangle rectBonus{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	_bonusSpawner->SpawnBonus(rectBonus, BonusType::Helmet);
	const auto shot{_allObjects.back()};

	shot->TakeDamage(1u, Author::Player1);

	EXPECT_FALSE(shot->GetIsAlive());
	EXPECT_EQ(_statistics->GetData().Seat(PlayerSlot::P1).bonusesDestroyed, 1u);
	EXPECT_EQ(_statistics->GetData().Seat(PlayerSlot::P1).bonusPickups, 0u);

	_bonusSpawner->SpawnBonus(rectBonus, BonusType::Helmet);
	const auto taken{std::dynamic_pointer_cast<Bonus>(_allObjects.back())};
	ASSERT_NE(taken, nullptr);

	taken->PickUpBonus(Author::Player1);

	EXPECT_FALSE(taken->GetIsAlive());
	EXPECT_EQ(_statistics->GetData().Seat(PlayerSlot::P1).bonusPickups, 1u);
	EXPECT_EQ(_statistics->GetData().Seat(PlayerSlot::P1).bonusesDestroyed, 1u);
}
