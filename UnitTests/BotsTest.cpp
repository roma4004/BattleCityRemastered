#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/GameStateManager.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/BulletCalibre.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "geometry/Point.h"
#include "gtest/gtest.h"
#include <chrono>
#include <cstddef>
#include <memory>

using namespace std::chrono_literals;

class BotsTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<GameStateManager> _stateManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	Uuid _uuid{};
	double _tankSize{};
	double _gridSize{};
	unsigned short _tankHealth{100u};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_stateManager = std::make_shared<GameStateManager>(_events);
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_gridSize = _gameConfig.gridOffset;
		_tankSize = _gameConfig.tankSize;

		//NOTE: both rolls are pinned open, or every test that expects a shot at an obstacle would flake
		_gameConfig.botShootObstacleChance = 1.0;
		_gameConfig.botShootFortressChance = 1.0;

		_allObjects.reserve(4u);
	}

	void TearDown() override {}

	std::shared_ptr<Tank> CreateBot(const FPoint pos, const Author author, const Direction dir,
									const unsigned short tier = 1u)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto bot{TestUtils::CreateBot(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool, _gameConfig,
									  tier)};

		return bot;
	}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author, const Direction dir)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool,
											_gameConfig)};

		return player;
	}

	std::shared_ptr<Bullet> CreateBullet(const FPoint pos, const Direction dir, const Author author,
										 const BulletCalibre& calibre)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = calibre.size.x, .h = calibre.size.y};
		auto bullet{TestUtils::CreateBullet(rect, _tankHealth, _bulletPool, _events, calibre, dir,
											author)};

		return bullet;
	}

	void SpawnObstacleArea(const ObjRectangle area, const ObstacleType type) const
	{
		TestUtils::SpawnObstacleArea(_events, _allObjects, area, type, _gameConfig);
	}

	//NOTE: a shot of the bot's own make - what matters here is that it flies at the game's speed, so the
	//time left to answer it is the game's too
	[[nodiscard]] static BulletCalibre IncomingCalibre(const Tank& tank)
	{
		return BulletCalibre{.speed = tank.GetBulletSpeed(),
							 .damage = 1u,
							 .damageRadius = tank.GetBulletDamageRadius(),
							 .tier = 1u,
							 .size{.x = tank.GetBulletWidth(), .y = tank.GetBulletHeight()}};
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const ObjRectangle rect, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, rect, type);
	}
};

// The one that decides first turns onto its opponent. The second decides after that shot exists, and a
// bullet on its way outranks the tank that sent it - so it answers the shot, not the shooter
TEST_F(BotsTest, BotsChangeDirectionIfOpponentSeen)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
	const auto enemyBot{CreateBot({.x = _tankSize * 3.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	const Direction startDirCoop{coopBot->GetDirection()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop{coopBot->GetDirection()};
	const Direction endDirEnemy{enemyBot->GetDirection()};

	EXPECT_NE(startDirCoop, endDirCoop);
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	// the shot travels along the row, so off its lane is up or down, and the top edge leaves only down
	EXPECT_EQ(endDirEnemy, Direction::DOWN);
}

// an ally in sight is no reason to turn the hull
TEST_F(BotsTest, BotsNoChangeDirectionIfPlayerAllySeen)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	CreatePlayer({.x = _tankSize * 3.0, .y = 0.0}, Author::Player2, Direction::DOWN);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN);
}

// nor is an allied bot
TEST_F(BotsTest, BotsNoChangeDirectionIfBotAllySeen)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	const auto secondCoopBot{CreateBot({.x = _tankSize * 3.0, .y = 0.0}, Author::Player2, Direction::DOWN)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN);
	EXPECT_EQ(secondCoopBot->GetDirection(), Direction::DOWN);
}

// A bot does not turn to an opponent it cannot shoot without catching its own blast
TEST_F(BotsTest, BotsNoChangeDirectionIfOpponentTooClose)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	const auto enemyBot{CreateBot({.x = _tankSize, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN);
	EXPECT_EQ(enemyBot->GetDirection(), Direction::DOWN);
}

// a bonus in sight, on the other hand, turns the bot towards it
TEST_F(BotsTest, BotsChangeDirectionIfBonusSeenAndNoOneShoot)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 21.0, .y = 0.0, .w = _tankSize, .h = _tankSize});

	const auto enemyBot{CreateBot({.x = _tankSize * 3.0 + 40.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	const Direction startDirCoop{coopBot->GetDirection()};
	const Direction startDirEnemy{enemyBot->GetDirection()};
	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop{coopBot->GetDirection()};
	const Direction endDirEnemy{enemyBot->GetDirection()};

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_NE(startDirCoop, endDirCoop);
	EXPECT_NE(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::LEFT);
}

// water is driven around, so a bonus behind it is not worth turning for
TEST_F(BotsTest, BotsCantSeeBonusBehindWater)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::RIGHT)};

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
				  ObstacleType::Water);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 4.0 + 3.0, .w = _tankSize, .h = _tankSize},
				  ObstacleType::Water);

	const auto enemyBot{CreateBot({.x = 0.0, .y = _tankSize * 5.0 + 40.0}, Author::Enemy1, Direction::RIGHT)};

	const Direction startDirCoop{coopBot->GetDirection()};
	const Direction startDirEnemy{enemyBot->GetDirection()};
	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop{coopBot->GetDirection()};
	const Direction endDirEnemy{enemyBot->GetDirection()};

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_EQ(startDirCoop, endDirCoop);
	EXPECT_EQ(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::RIGHT);
}

// a bush hides one outright
TEST_F(BotsTest, BotsCantSeeBonusBehindBush)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::RIGHT)};

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
				  ObstacleType::Bush);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 4.0 + 3.0, .w = _tankSize, .h = _tankSize},
				  ObstacleType::Bush);

	const auto enemyBot{CreateBot({.x = 0.0, .y = _tankSize * 5.0 + 40.0}, Author::Enemy1, Direction::RIGHT)};

	const Direction startDirCoop{coopBot->GetDirection()};
	const Direction startDirEnemy{enemyBot->GetDirection()};
	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop{coopBot->GetDirection()};
	const Direction endDirEnemy{enemyBot->GetDirection()};

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_EQ(startDirCoop, endDirCoop);
	EXPECT_EQ(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::RIGHT);
}

// ice hides nothing and is driven over
TEST_F(BotsTest, BotsCanSeeBonusBehindIce)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::RIGHT)};

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
				  ObstacleType::Ice);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 4.0 + 3.0, .w = _tankSize, .h = _tankSize},
				  ObstacleType::Ice);

	const auto enemyBot{CreateBot({.x = 0.0, .y = _tankSize * 5.0 + 40.0}, Author::Enemy1, Direction::RIGHT)};

	const Direction startDirCoop{coopBot->GetDirection()};
	const Direction startDirEnemy{enemyBot->GetDirection()};
	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop{coopBot->GetDirection()};
	const Direction endDirEnemy{enemyBot->GetDirection()};

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_NE(startDirCoop, endDirCoop);
	EXPECT_NE(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::DOWN);
	EXPECT_EQ(endDirEnemy, Direction::UP);
}

// and a bonus lying on the ice is reached the same way
TEST_F(BotsTest, BotsCanSeeBonusInTheIce)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::RIGHT)};

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
				  ObstacleType::Ice);

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 3.0 + 1.0, .w = _tankSize, .h = _tankSize},
				  ObstacleType::Ice);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 4.0 + 3.0, .w = _tankSize, .h = _tankSize},
				  ObstacleType::Ice);

	const auto enemyBot{CreateBot({.x = 0.0, .y = _tankSize * 5.0 + 40.0}, Author::Enemy1, Direction::RIGHT)};

	const Direction startDirCoop{coopBot->GetDirection()};
	const Direction startDirEnemy{enemyBot->GetDirection()};
	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop{coopBot->GetDirection()};
	const Direction endDirEnemy{enemyBot->GetDirection()};

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_NE(startDirCoop, endDirCoop);
	EXPECT_NE(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::DOWN);
	EXPECT_EQ(endDirEnemy, Direction::UP);
}

// A shot already on our line is answered with a shot, and the hull is left where it was - turning to
// face a bullet is driving at it, which is what the old behaviour did
TEST_F(BotsTest, BotShootsDownAnIncomingBulletWithoutTurning)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::RIGHT)};

	const double bulletHeight{coopBot->GetBulletHeight()};

	// head-on: to the right of the bot, flying at it, and far enough that a shot still meets it
	CreateBullet({.x = _tankSize * 4.0, .y = (_tankSize - bulletHeight) / 2.0}, Direction::LEFT, Author::Enemy1,
				 IncomingCalibre(*coopBot));

	const std::size_t worldSizeBeforeShot{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::RIGHT) << "the hull turned towards the bullet";
	EXPECT_GT(_allObjects.size(), worldSizeBeforeShot) << "nothing was fired at it";
}

// Where it is headed is the whole question now: one flying away is somebody else's problem
TEST_F(BotsTest, BotIgnoresABulletFlyingAway)
{
	//NOTE: facing along the bullet's own axis on purpose - a dodge would go across it, so taking this
	//shot for a threat shows up as a turn and cannot hide behind the heading the bot already had
	const auto coopBot{CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::RIGHT)};

	const double bulletHeight{coopBot->GetBulletHeight()};
	CreateBullet({.x = _tankSize * 2.0, .y = _tankSize * 3.0 + (_tankSize - bulletHeight) / 2.0},
				 Direction::RIGHT, Author::Enemy1, IncomingCalibre(*coopBot));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::RIGHT) << "a bullet leaving was taken for a threat";
}

// A shot that was never going to reach us: same direction, a row over. Nothing to answer, so nothing
// changes - a bot that dodged every bullet on the field would never drive anywhere
TEST_F(BotsTest, BotIgnoresABulletInAnotherLane)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::DOWN)};

	const double bulletHeight{coopBot->GetBulletHeight()};
	// two tanks higher up: flying left, past the bot rather than into it
	CreateBullet({.x = _tankSize * 4.0, .y = _tankSize + (_tankSize - bulletHeight) / 2.0}, Direction::LEFT,
				 Author::Enemy1, IncomingCalibre(*coopBot));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN) << "a bullet in another row was dodged";
}

// Crossing our lane rather than coming down it: there is nothing to shoot at, so the answer is to get
// off the line, and off it means across the bullet's path and not across our own heading
TEST_F(BotsTest, BotStepsOutOfTheLaneOfACrossingBullet)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::DOWN)};

	const double bulletHeight{coopBot->GetBulletHeight()};
	// to the right of the bot and flying left, so it arrives across the way the bot is looking
	CreateBullet({.x = _tankSize * 4.0, .y = _tankSize * 3.0 + (_tankSize - bulletHeight) / 2.0},
				 Direction::LEFT, Author::Enemy1, IncomingCalibre(*coopBot));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction dodged{coopBot->GetDirection()};

	EXPECT_TRUE(dodged == Direction::UP || dodged == Direction::DOWN)
			<< "the dodge went along the bullet's lane instead of out of it";
}

// Of the two ways out, the one with room: a wall right above leaves only downwards
TEST_F(BotsTest, BotDodgesTowardsTheSideWithMoreRoom)
{
	const auto coopBot{CreateBot({.x = _tankSize * 4.0, .y = _tankSize * 3.0}, Author::Player1, Direction::DOWN)};

	SpawnObstacleArea(ObjRectangle{.x = _tankSize * 4.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize},
					  ObstacleType::Steel);

	const double bulletHeight{coopBot->GetBulletHeight()};
	CreateBullet({.x = _tankSize * 8.0, .y = _tankSize * 3.0 + (_tankSize - bulletHeight) / 2.0},
				 Direction::LEFT, Author::Enemy1, IncomingCalibre(*coopBot));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN) << "it dodged into the wall above";
}

// Head-on, but there is no time left for two bullets to meet - so the answer stops being a shot
TEST_F(BotsTest, ABulletTooCloseIsDodgedRatherThanShot)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::RIGHT)};

	const double bulletHeight{coopBot->GetBulletHeight()};
	CreateBullet({.x = _tankSize + 1.0, .y = _tankSize * 3.0 + (_tankSize - bulletHeight) / 2.0},
				 Direction::LEFT, Author::Enemy1, IncomingCalibre(*coopBot));

	const std::size_t worldSizeBeforeShot{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction dodged{coopBot->GetDirection()};

	EXPECT_TRUE(dodged == Direction::UP || dodged == Direction::DOWN) << "it stood and fired point blank";
	EXPECT_EQ(_allObjects.size(), worldSizeBeforeShot) << "a shot went out with no time to arrive";
}

// The cooldown gate lives in Tank::TickUpdate, not in ShouldShoot - one shot per cooldown, no matter
// how long the target stays in sight
TEST_F(BotsTest, BotDoesNotShootTwiceWithinOneCooldown)
{
	const auto bot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::RIGHT)};
	CreateBot({.x = _tankSize * 3.0, .y = 0.0}, Author::Enemy1, Direction::LEFT);

	const std::size_t beforeFirstShot{_allObjects.size()};
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	const std::size_t afterFirstShot{_allObjects.size()};
	ASSERT_GT(afterFirstShot, beforeFirstShot);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_allObjects.size(), afterFirstShot);
}

// The cooldown suppresses aiming, not only firing: ChangeDirIfSeenOpponent bails on !CanShoot(). With a
// shot on its way back the reloading bot has one answer left, and it is to leave the line
TEST_F(BotsTest, AReloadingBotStepsAsideFromTheShotComingBack)
{
	const auto bot{CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::RIGHT)};
	CreateBot({.x = _tankSize * 3.0, .y = _tankSize * 3.0}, Author::Enemy1, Direction::LEFT);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_EQ(bot->GetDirection(), Direction::RIGHT) << "the first frame is the exchange of shots";

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction dodged{bot->GetDirection()};

	EXPECT_TRUE(dodged == Direction::UP || dodged == Direction::DOWN)
			<< "a reloading bot stood in the lane of the answer";
}

// Nose to the wall: the move fails and steel cannot be shot, so the only way out is to turn. The gap
// is the padding movement parks on - flush against the wall every side reads as blocked
TEST_F(BotsTest, BotTurnsWhenItRunsIntoAWall)
{
	const auto bot{CreateBot({.x = _tankSize * 2.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT)};

	// flush against the bot's right side, so the very first step is blocked
	SpawnObstacleArea(ObjRectangle{.x = _tankSize * 3.0 + 1.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize},
					  ObstacleType::Steel);

	ASSERT_EQ(bot->GetDirection(), Direction::RIGHT);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(bot->GetDirection(), Direction::RIGHT);
}

// A bot must not fire into something closer than its own blast radius. Walled in on all four sides,
// so no random turn can move the target out of the way.
TEST_F(BotsTest, BotHoldsFireWhenTheBlastWouldReachItself)
{
	const auto bot{CreateBot({.x = _tankSize, .y = _tankSize}, Author::Enemy1, Direction::RIGHT)};

	for (const ObjRectangle& wallRect: {ObjRectangle{.x = _tankSize * 2.0, .y = _tankSize,
													   .w = _tankSize, .h = _tankSize},
										   ObjRectangle{.x = 0.0, .y = _tankSize, .w = _tankSize, .h = _tankSize},
										   ObjRectangle{.x = _tankSize, .y = 0.0, .w = _tankSize, .h = _tankSize},
										   ObjRectangle{.x = _tankSize, .y = _tankSize * 2.0,
														.w = _tankSize, .h = _tankSize}})
	{
		SpawnObstacle(wallRect, ObstacleType::Water);
	}

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bot->GetDirection(), Direction::RIGHT);
	EXPECT_EQ(_allObjects.size(), before);
}

// A bot has no keyboard to ignore, so Deactivate has to drop the tick subscription itself
TEST_F(BotsTest, ADeactivatedBotDoesNotDrive)
{
	const auto bot{CreateBot({.x = _tankSize * 3.0, .y = _tankSize * 3.0}, Author::Enemy1, Direction::DOWN)};

	const FPoint start{bot->GetPos()};
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_NE(bot->GetPos(), start) << "the control failed - this bot does not drive even when active";

	bot->Deactivate();
	const FPoint parked{bot->GetPos()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bot->GetPos(), parked);
}

// The chance gates walls and nothing else - a bot with a target in its sights fires whatever the
// wall roll would have said
TEST_F(BotsTest, BotShootsAnOpponentEvenWithTheWallChanceAtZero)
{
	_gameConfig.botShootObstacleChance = 0.0;

	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::RIGHT);
	CreateBot({.x = _tankSize * 3.0, .y = 0.0}, Author::Enemy1, Direction::LEFT);

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(_allObjects.size(), before);
}

// A wall in plain sight and in range, and still no shot - the roll is what decides, and at zero it
// decides once per sighting, so no number of frames wears it down
TEST_F(BotsTest, BotHoldsFireAtAWallWhenTheChanceIsZero)
{
	_gameConfig.botShootObstacleChance = 0.0;

	const auto bot{CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize},
					  ObstacleType::Brick);

	const std::size_t before{_allObjects.size()};

	for (int frame{}; frame < 10; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_EQ(_allObjects.size(), before);
	ASSERT_EQ(bot->GetDirection(), Direction::DOWN) << "the control failed - the bot looked away from the wall";
}

// with the obstacle roll pinned open the wall ahead is fired at every time
TEST_F(BotsTest, BotShootsAWallWhenTheChanceIsOne)
{
	_gameConfig.botShootObstacleChance = 1.0;

	const auto bot{CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize},
					  ObstacleType::Brick);

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(_allObjects.size(), before);
}

// A bot counts as blocked only flush against the wall, and that close its own blast would reach it -
// the way out of a dead end is the turn, never the gun.
TEST_F(BotsTest, ABlockedBotStillTurnsAwayAtZeroChance)
{
	_gameConfig.botShootObstacleChance = 0.0;

	const auto bot{CreateBot({.x = _tankSize * 2.0, .y = _tankSize * 2.0}, Author::Enemy1, Direction::RIGHT)};

	// flush against the bot's right side, destructible, and still not worth a shot from this close
	SpawnObstacle(ObjRectangle{.x = _tankSize * 3.0 + 1.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize},
				  ObstacleType::Brick);

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(bot->GetDirection(), Direction::RIGHT);
	EXPECT_EQ(_allObjects.size(), before);
}

// A refusal has to expire, or one unlucky roll decides for as long as that wall is in the sights.
// Cooldown set to nothing, so the next frame asks again with the chance flipped to the other end.
TEST_F(BotsTest, ARefusedWallIsReconsideredOnceTheCooldownIsUp)
{
	_gameConfig.botShootObstacleChance = 0.0;
	_gameConfig.botObstacleShootCooldown = 0ms;

	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize}, ObstacleType::Brick);

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_EQ(_allObjects.size(), before) << "the control failed - a zero chance fired";

	_gameConfig.botShootObstacleChance = 1.0;
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(_allObjects.size(), before) << "the refusal outlived its cooldown";
}

// And the other half: until that cooldown is up the answer stands, or the roll is back to per frame
TEST_F(BotsTest, AWallRefusedStaysRefusedUntilTheCooldownIsUp)
{
	_gameConfig.botShootObstacleChance = 0.0;
	_gameConfig.botObstacleShootCooldown = 1min;

	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize},
					  ObstacleType::Brick);

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	_gameConfig.botShootObstacleChance = 1.0;

	for (int frame{}; frame < 10; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_EQ(_allObjects.size(), before) << "the answer was re-rolled before its cooldown was up";
}
