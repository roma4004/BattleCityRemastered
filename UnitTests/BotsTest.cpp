#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/GameStateManager.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/EagleTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/BulletCaliber.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "geometry/Point.h"
#include "utils/ObjectUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <array>
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
		//NOTE: and the bot sees at once - these tests read single frames, and noticing takes seconds of play
		_gameConfig.botNoticeDelayFactor = 0.0;

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
										 const BulletCaliber& caliber)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = caliber.size.x, .h = caliber.size.y};
		auto bullet{TestUtils::CreateBullet(rect, _bulletPool, _events, caliber, dir,
											author)};

		return bullet;
	}

	void SpawnObstacleArea(const ObjRectangle area, const ObstacleType type) const
	{
		TestUtils::SpawnObstacleArea(_events, _allObjects, area, type, _gameConfig);
	}

	//NOTE: a shot of the bot's own make - it flies at the game's speed, so the time to answer is the game's too
	[[nodiscard]] static BulletCaliber IncomingCaliber(const Tank& tank)
	{
		return BulletCaliber{.speed = tank.GetBulletSpeed(),
							 .damage = 1u,
							 .health = 1u,
							 .blastRadius = tank.GetBulletBlastRadius(),
							 .tier = 1u,
							 .size{.x = tank.GetBulletWidth(), .y = tank.GetBulletHeight()}};
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const ObjRectangle rect, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, rect, type);
	}
};

// a bot turns onto an opponent it sees down the row
TEST_F(BotsTest, BotsChangeDirectionIfOpponentSeen)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
	CreateBot({.x = _tankSize * 3.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::RIGHT);
}

// the second bot decides once the shot exists and answers it, not its shooter - the top edge leaves only down
TEST_F(BotsTest, ABotAnswersTheShotOnItsWayRatherThanTheShooter)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);
	const auto enemyBot{CreateBot({.x = _tankSize * 3.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetDirection(), Direction::DOWN);
}

// an ally in sight is no reason to turn the hull, a player or a bot alike
TEST_F(BotsTest, BotsNoChangeDirectionIfAllySeen)
{
	constexpr std::array kPlayerOrBot{false, true};
	for (const bool isAllyABot: kPlayerOrBot)
	{
		SCOPED_TRACE(isAllyABot);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
		const FPoint allyPos{.x = _tankSize * 3.0, .y = 0.0};
		if (isAllyABot)
		{
			CreateBot(allyPos, Author::Player2, Direction::DOWN);
		}
		else
		{
			CreatePlayer(allyPos, Author::Player2, Direction::DOWN);
		}

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN);
	}
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

// Water stops a tank, not a bullet: the bot holds a shot across it and turns away once the sights are empty
TEST_F(BotsTest, BotTurnsAwayOnceTheTargetItLinedUpOnIsGone)
{
	//NOTE: an off-center opponent is an obstacle, and a roll firing at it would free the hull - not what is read here
	_gameConfig.botShootObstacleChance = 0.0;

	const auto bot{CreateBot({.x = _tankSize * 3.0, .y = _tankSize * 4.0}, Author::Enemy1, Direction::RIGHT)};

	// flush above the bot, and skipped by the sights the way a bullet skips it
	SpawnObstacleArea(ObjRectangle{.x = _tankSize * 3.0, .y = _tankSize * 4.0 - _gridSize, .w = _tankSize,
								   .h = _gridSize}, ObstacleType::Water);

	// off center by a cell, so the bot lines up on it without ever taking the shot
	const auto target{CreatePlayer({.x = _tankSize * 3.0 + _gridSize, .y = _tankSize}, Author::Player1,
								   Direction::DOWN)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_EQ(bot->GetDirection(), Direction::UP) << "it never lined up on the tank across the water";

	target->SetIsAlive(false);

	//NOTE: two ticks - the sights are read at the end of one, so the hull is freed on the next
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(bot->GetDirection(), Direction::UP) << "it kept the heading of a tank that is no longer there";
}

// a bonus in sight, on the other hand, turns the bot towards it
TEST_F(BotsTest, BotsChangeDirectionIfBonusSeenAndNoOneShoot)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 21.0, .y = 0.0, .w = _tankSize, .h = _tankSize});

	const auto enemyBot{CreateBot({.x = _tankSize * 3.0 + 40.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop{coopBot->GetDirection()};
	const Direction endDirEnemy{enemyBot->GetDirection()};

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::LEFT);
}

// but not while an opponent it can shoot stands in the sights - the bonus keeps, the shot does not
TEST_F(BotsTest, ABonusAtTheSideDoesNotTurnABotOffAnOpponentItCanShoot)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Enemy1, Direction::DOWN);

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 21.0, .y = 0.0, .w = _tankSize, .h = _tankSize});

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN) << "it turned off a loaded shot for the bonus";
}

// and once the shot is away the bonus is the thing to do with the reload
TEST_F(BotsTest, AReloadingBotGoesForTheBonusItPassedUp)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Enemy1, Direction::DOWN);

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 21.0, .y = 0.0, .w = _tankSize, .h = _tankSize});

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::RIGHT) << "the gun is on cooldown and the bonus still waits";
}

// a bonus down the column is seen across ice, in it or behind it - not behind water (driven around) or a bush
TEST_F(BotsTest, ABonusIsSeenAcrossIceButNotBehindWaterOrABush)
{
	struct Sight final
	{
		ObstacleType terrain;
		bool isBonusOnIt;
		Direction turnedTo;
	};

	constexpr std::array kSights{
			 Sight{.terrain = ObstacleType::Water, .isBonusOnIt = false, .turnedTo = Direction::RIGHT},
			 Sight{.terrain = ObstacleType::Bush, .isBonusOnIt = false, .turnedTo = Direction::RIGHT},
			 Sight{.terrain = ObstacleType::Ice, .isBonusOnIt = false, .turnedTo = Direction::DOWN},
			 Sight{.terrain = ObstacleType::Ice, .isBonusOnIt = true, .turnedTo = Direction::DOWN}};
	for (const auto& [terrain, isBonusOnIt, turnedTo]: kSights)
	{
		SCOPED_TRACE(testing::Message() << static_cast<int>(terrain) << (isBonusOnIt ? " under" : " in front"));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::RIGHT)};
		SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize}, terrain);
		if (isBonusOnIt)
		{
			SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 3.0 + 1.0, .w = _tankSize, .h = _tankSize},
						  terrain);
		}

		_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(coopBot->GetDirection(), turnedTo);
	}
}

// a bonus off the line it drives is no reason to turn
TEST_F(BotsTest, BotsNoChangeDirectionIfBonusOutsideLineOfSight)
{
	const auto bot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
	_bonusSpawner->SpawnRandomBonus({.x = _tankSize * 2.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize});

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bot->GetDirection(), Direction::DOWN);
}

// a shot coming down our line is answered with a shot, hull kept - turning to face a bullet is driving at it
TEST_F(BotsTest, BotShootsDownAnIncomingBulletWithoutTurning)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::RIGHT)};

	const double bulletHeight{coopBot->GetBulletHeight()};

	// head-on: to the right of the bot, flying at it, and far enough that a shot still meets it
	CreateBullet({.x = _tankSize * 4.0, .y = (_tankSize - bulletHeight) / 2.0}, Direction::LEFT, Author::Enemy1,
				 IncomingCaliber(*coopBot));

	const std::size_t worldSizeBeforeShot{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::RIGHT) << "the hull turned towards the bullet";
	EXPECT_GT(_allObjects.size(), worldSizeBeforeShot) << "nothing was fired at it";
}

// Where it is headed is the whole question now: one flying away is somebody else's problem
TEST_F(BotsTest, BotIgnoresABulletFlyingAway)
{
	//NOTE: facing along the bullet's axis on purpose - a dodge goes across it, so a false alarm shows as a turn
	const auto coopBot{CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::RIGHT)};

	const double bulletHeight{coopBot->GetBulletHeight()};
	CreateBullet({.x = _tankSize * 2.0, .y = _tankSize * 3.0 + (_tankSize - bulletHeight) / 2.0},
				 Direction::RIGHT, Author::Enemy1, IncomingCaliber(*coopBot));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::RIGHT) << "a bullet leaving was taken for a threat";
}

// a shot a row over in the same direction never reaches us - a bot dodging every bullet would never drive
TEST_F(BotsTest, BotIgnoresABulletInAnotherLane)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::DOWN)};

	const double bulletHeight{coopBot->GetBulletHeight()};
	// two tanks higher up: flying left, past the bot rather than into it
	CreateBullet({.x = _tankSize * 4.0, .y = _tankSize + (_tankSize - bulletHeight) / 2.0}, Direction::LEFT,
				 Author::Enemy1, IncomingCaliber(*coopBot));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN) << "a bullet in another row was dodged";
}

// a bullet crossing our lane is nothing to shoot - the answer is to leave the line across its path
TEST_F(BotsTest, BotStepsOutOfTheLaneOfACrossingBullet)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::DOWN)};

	const double bulletHeight{coopBot->GetBulletHeight()};
	// to the right of the bot and flying left, so it arrives across the way the bot is looking
	CreateBullet({.x = _tankSize * 4.0, .y = _tankSize * 3.0 + (_tankSize - bulletHeight) / 2.0},
				 Direction::LEFT, Author::Enemy1, IncomingCaliber(*coopBot));

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
				 Direction::LEFT, Author::Enemy1, IncomingCaliber(*coopBot));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN) << "it dodged into the wall above";
}

// A dodge has to end outside the lane, and this narrow a passage has no room for it - so the hull stays put
TEST_F(BotsTest, BotDoesNotDodgeWhereThereIsNoRoomToLeaveTheLane)
{
	const double gap{_gridSize / 2.0};
	const double laneY{_gridSize * 5.0 + gap};
	const auto bot{CreateBot({.x = _tankSize * 3.0, .y = laneY}, Author::Enemy1, Direction::RIGHT)};

	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = _gridSize * 4.0, .w = _tankSize * 8.0, .h = _gridSize},
					  ObstacleType::Steel);
	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = laneY + _tankSize + gap, .w = _tankSize * 8.0, .h = _gridSize},
					  ObstacleType::Steel);

	// close enough that there is no time to shoot it down either, or the intercept would hold the hull
	const double bulletHeight{bot->GetBulletHeight()};
	CreateBullet({.x = _tankSize * 4.0 + _gridSize * 2.0, .y = laneY + (_tankSize - bulletHeight) / 2.0},
				 Direction::LEFT, Author::Player1, IncomingCaliber(*bot));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bot->GetDirection(), Direction::RIGHT) << "it turned into a gap it cannot leave the lane by";
}

// Head-on, but there is no time left for two bullets to meet - so the answer stops being a shot
TEST_F(BotsTest, ABulletTooCloseIsDodgedRatherThanShot)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::RIGHT)};

	const double bulletHeight{coopBot->GetBulletHeight()};
	CreateBullet({.x = _tankSize + 1.0, .y = _tankSize * 3.0 + (_tankSize - bulletHeight) / 2.0},
				 Direction::LEFT, Author::Enemy1, IncomingCaliber(*coopBot));

	const std::size_t worldSizeBeforeShot{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction dodged{coopBot->GetDirection()};

	EXPECT_TRUE(dodged == Direction::UP || dodged == Direction::DOWN) << "it stood and fired point blank";
	EXPECT_EQ(_allObjects.size(), worldSizeBeforeShot) << "a shot went out with no time to arrive";
}

// reloading suppresses aiming too, so with a shot coming back the bot's one answer is to leave the line
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

// whom a bot fires at is decided by sides; the wall rolls are shut, and only the shooter's shots are counted
TEST_F(BotsTest, ABotShootsAnOpponentDownTheLine)
{
	struct Duel final
	{
		GameMode mode;
		Author shooter;
		Author target;
		bool isTargetABot{};
	};

	_gameConfig.botShootObstacleChance = 0.0;
	_gameConfig.botShootFortressChance = 0.0;
	constexpr std::array kDuels{
			 Duel{.mode = GameMode::OnePlayer, .shooter = Author::Enemy1, .target = Author::Player1},
			 Duel{.mode = GameMode::TwoPlayers, .shooter = Author::Enemy1, .target = Author::Player2},
			 Duel{.mode = GameMode::CoopWithBot, .shooter = Author::Enemy1, .target = Author::Player2,
				  .isTargetABot = true},
			 Duel{.mode = GameMode::CoopWithBot, .shooter = Author::Player2, .target = Author::Enemy1,
				  .isTargetABot = true},
			 Duel{.mode = GameMode::FreeForAll, .shooter = Author::Enemy1, .target = Author::Enemy2,
				  .isTargetABot = true}};
	for (const auto& [mode, shooter, target, isTargetABot]: kDuels)
	{
		SCOPED_TRACE(testing::Message() << static_cast<int>(shooter) << " at " << static_cast<int>(target));
		_gameConfig.gameMode = mode;
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		CreateBot({.x = 0.0, .y = 0.0}, shooter, Direction::DOWN);
		const FPoint targetPos{.x = 0.0, .y = _tankSize * 3.0};
		if (isTargetABot)
		{
			CreateBot(targetPos, target, Direction::DOWN);
		}
		else
		{
			CreatePlayer(targetPos, target, Direction::UP);
		}

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		const auto shots{std::ranges::count_if(_allObjects, [shooter](const std::shared_ptr<BaseObj>& object)
		{
			const Bullet* bullet{dynamic_cast<const Bullet*>(object.get())};

			return bullet != nullptr && bullet->GetAuthor() == shooter;
		})};
		EXPECT_EQ(shots, 1);
	}
}

// its own side is no target, rolls open: an enemy spares an enemy, a player's bot its ally and its player
TEST_F(BotsTest, ABotHoldsFireAtItsOwnSide)
{
	struct Ally final
	{
		Author shooter;
		Author ally;
		bool isAllyABot{};
	};

	constexpr std::array kAllies{
			 Ally{.shooter = Author::Enemy1, .ally = Author::Enemy2, .isAllyABot = true},
			 Ally{.shooter = Author::Player2, .ally = Author::Player1, .isAllyABot = true},
			 Ally{.shooter = Author::Player2, .ally = Author::Player1}};
	for (const auto& [shooter, ally, isAllyABot]: kAllies)
	{
		SCOPED_TRACE(testing::Message() << static_cast<int>(shooter) << " and " << static_cast<int>(ally));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		CreateBot({.x = 0.0, .y = 0.0}, shooter, Direction::DOWN);
		const FPoint allyPos{.x = 0.0, .y = _tankSize * 3.0};
		if (isAllyABot)
		{
			CreateBot(allyPos, ally, Direction::DOWN);
		}
		else
		{
			CreatePlayer(allyPos, ally, Direction::UP);
		}

		const std::size_t before{_allObjects.size()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_allObjects.size(), before);
	}
}

// no shot at a wall with an ally or a bonus behind it, however many walls deep; one off the fire line is no reason
TEST_F(BotsTest, ABotHoldsFireAtAWallWhenTheShotWouldTakeAnAllyOrABonus)
{
	struct Behind final
	{
		bool isBonus{};
		double wallCells{};
		bool isAside{};
		bool isShot{};
	};

	constexpr std::array kRows{Behind{.wallCells = 1.0},
							   Behind{.isBonus = true, .wallCells = 1.0},
							   Behind{.wallCells = 3.0},
							   Behind{.isBonus = true, .wallCells = 3.0},
							   Behind{.wallCells = 3.0, .isAside = true, .isShot = true},
							   Behind{.isBonus = true, .wallCells = 3.0, .isAside = true, .isShot = true}};
	for (const auto& [isBonus, wallCells, isAside, isShot]: kRows)
	{
		SCOPED_TRACE(testing::Message() << (isBonus ? "bonus" : "ally") << (isAside ? " aside of " : " behind ")
										<< wallCells << " cells");
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
		const ObjRectangle wall{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _gridSize * wallCells};
		SpawnObstacleArea(wall, ObstacleType::Brick);
		const FPoint behind{.x = isAside ? _tankSize * 3.0 : 0.0, .y = wall.y + wall.h};
		if (isBonus)
		{
			_bonusSpawner->SpawnBonus({.x = behind.x, .y = behind.y, .w = _tankSize, .h = _tankSize},
									  BonusType::Helmet);
		}
		else
		{
			CreateBot(behind, Author::Enemy2, Direction::DOWN);
		}

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		const auto shots{std::ranges::count_if(_allObjects, [](const std::shared_ptr<BaseObj>& object)
		{
			const Bullet* const bullet{dynamic_cast<const Bullet*>(object.get())};

			return bullet != nullptr && bullet->GetAuthor() == Author::Enemy1;
		})};
		EXPECT_EQ(shots, isShot ? 1 : 0);
	}
}

// the base is the enemy's to take: an enemy fires at the eagle and its wall, a player's bot holds fire at both
TEST_F(BotsTest, AnEnemyShootsTheBaseAndAPlayersBotSparesIt)
{
	struct Base final
	{
		Author shooter;
		ObstacleType part;
		bool isShot{};
	};

	constexpr std::array kBaseParts{
			 Base{.shooter = Author::Enemy1, .part = ObstacleType::Eagle, .isShot = true},
			 Base{.shooter = Author::Enemy1, .part = ObstacleType::Fortress, .isShot = true},
			 Base{.shooter = Author::Player2, .part = ObstacleType::Eagle},
			 Base{.shooter = Author::Player2, .part = ObstacleType::Fortress}};
	for (const auto& [shooter, part, isShot]: kBaseParts)
	{
		SCOPED_TRACE(testing::Message() << static_cast<int>(shooter) << " at " << static_cast<int>(part));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		CreateBot({.x = 0.0, .y = 0.0}, shooter, Direction::DOWN);
		SpawnObstacleArea({.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize}, part);

		const std::size_t before{_allObjects.size()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_allObjects.size(), before + (isShot ? 1u : 0u));
	}
}

// water and ice hide nobody - a bullet crosses them, so a player behind them or standing in them is in sight
TEST_F(BotsTest, BotShootsAPlayerBehindOrInWaterAndIce)
{
	const std::array placements{std::pair{_tankSize * 2.0 + 1.0, _tankSize * 3.0 + 2.0},
								std::pair{_tankSize * 3.0 + 1.0, _tankSize * 3.0 + 1.0}};
	constexpr std::array kTerrains{ObstacleType::Water, ObstacleType::Ice};
	for (const ObstacleType terrain: kTerrains)
	{
		for (const auto& [terrainY, playerY]: placements)
		{
			SCOPED_TRACE(testing::Message() << static_cast<int>(terrain) << " at " << terrainY);
			_events->EmitEvent(GameResetEvent{});
			_allObjects.clear();
			CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
			SpawnObstacleArea({.x = 0.0, .y = terrainY, .w = _tankSize, .h = _tankSize}, terrain);
			CreatePlayer({.x = 0.0, .y = playerY}, Author::Player1, Direction::UP);

			const std::size_t before{_allObjects.size()};

			_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

			EXPECT_EQ(_allObjects.size(), before + 1u);
		}
	}
}

// a wall between them breaks the line of sight - brick, steel and the eagle's own wall alike
TEST_F(BotsTest, BotHoldsFireAtAPlayerBehindAWall)
{
	//NOTE: the rolls shut - the bot fires at a player in sight and at nothing else
	_gameConfig.botShootObstacleChance = 0.0;
	_gameConfig.botShootFortressChance = 0.0;
	constexpr std::array kWalls{ObstacleType::Brick, ObstacleType::Steel, ObstacleType::Fortress};
	for (const ObstacleType wall: kWalls)
	{
		SCOPED_TRACE(static_cast<int>(wall));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
		SpawnObstacleArea({.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize}, wall);
		CreatePlayer({.x = 0.0, .y = _tankSize * 3.0 + 2.0}, Author::Player1, Direction::UP);

		const std::size_t before{_allObjects.size()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_allObjects.size(), before);
	}
}

// a bush is not solid, but it hides a player behind it or standing inside it
TEST_F(BotsTest, BotHoldsFireAtAPlayerBehindOrInABush)
{
	const std::array placements{std::pair{_tankSize * 2.0 + 1.0, _tankSize * 3.0 + 2.0},
								std::pair{_tankSize * 3.0 + 1.0, _tankSize * 3.0 + 1.0}};
	for (const auto& [bushY, playerY]: placements)
	{
		SCOPED_TRACE(bushY);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
		SpawnObstacleArea({.x = 0.0, .y = bushY, .w = _tankSize, .h = _tankSize}, ObstacleType::Bush);
		CreatePlayer({.x = 0.0, .y = playerY}, Author::Player1, Direction::UP);

		const std::size_t before{_allObjects.size()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_allObjects.size(), before);
	}
}

// one shot per cooldown however long the target stays in sight - the gate is in Tank::TickUpdate
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

// no fire into anything inside its own blast radius - walled in on four sides, so no turn moves the target
TEST_F(BotsTest, BotHoldsFireWhenTheBlastWouldReachItself)
{
	const auto bot{CreateBot({.x = _tankSize, .y = _tankSize}, Author::Enemy1, Direction::RIGHT)};

	const std::array walls{ObjRectangle{.x = _tankSize * 2.0, .y = _tankSize,
										  .w = _tankSize, .h = _tankSize},
							  ObjRectangle{.x = 0.0, .y = _tankSize, .w = _tankSize, .h = _tankSize},
							  ObjRectangle{.x = _tankSize, .y = 0.0, .w = _tankSize, .h = _tankSize},
							  ObjRectangle{.x = _tankSize, .y = _tankSize * 2.0,
										   .w = _tankSize, .h = _tankSize}};
	for (const ObjRectangle& wallRect: walls)
	{
		SpawnObstacle(wallRect, ObstacleType::Water);
	}

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bot->GetDirection(), Direction::RIGHT);
	EXPECT_EQ(_allObjects.size(), before);
}

// a wall a muzzle away is left alone
TEST_F(BotsTest, BotTooCloseToShootTheBrick)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
	SpawnObstacleArea({.x = 0.0, .y = _tankSize + 3.0, .w = _tankSize, .h = _tankSize}, ObstacleType::Brick);

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_allObjects.size(), before);
}

// terrain is no target with both rolls open: water, a bush and ice draw no fire
TEST_F(BotsTest, BotHoldsFireAtWaterBushAndIce)
{
	constexpr std::array kTerrains{ObstacleType::Water, ObstacleType::Bush, ObstacleType::Ice};
	for (const ObstacleType terrain: kTerrains)
	{
		SCOPED_TRACE(static_cast<int>(terrain));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
		SpawnObstacleArea({.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize}, terrain);

		const std::size_t before{_allObjects.size()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_allObjects.size(), before);
	}
}

// nose to steel the move fails and the wall can't be shot, so it turns - the gap is where movement parks
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

// steel draws fire only from the tier that breaks it - a cell farther off, the bot holds fire inside its blast
TEST_F(BotsTest, ABotShootsSteelOnlyFromTheTierThatBreaksIt)
{
	struct Gun final
	{
		unsigned short tier;
		bool isShot{};
	};

	constexpr std::array kGuns{Gun{.tier = 1u}, Gun{.tier = 3u, .isShot = true}};
	for (const auto& [tier, isShot]: kGuns)
	{
		SCOPED_TRACE(tier);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN, tier);
		SpawnObstacleArea({.x = 0.0, .y = _tankSize * 3.0 + _gridSize, .w = _tankSize, .h = _tankSize},
						  ObstacleType::Steel);

		const std::size_t before{_allObjects.size()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_allObjects.size(), before + (isShot ? 1u : 0u));
	}
}

// a wall in sight and range, roll at zero: no shot, and the roll is taken once per sighting, not every frame
TEST_F(BotsTest, BotHoldsFireAtAWallWhenTheChanceIsZero)
{
	_gameConfig.botShootObstacleChance = 0.0;

	const auto bot{CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize},
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

	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize},
					  ObstacleType::Brick);

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(_allObjects.size(), before);
}

// a bot flush against a wall would catch its own blast - the way out of a dead end is the turn, never the gun
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

// a refusal expires: with no cooldown the next frame asks again, here with the chance flipped to one
TEST_F(BotsTest, ARefusedWallIsReconsideredOnceTheCooldownIsUp)
{
	_gameConfig.botShootObstacleChance = 0.0;
	_gameConfig.botObstacleShootCooldown = 0ms;

	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize}, ObstacleType::Brick);

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

	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize},
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

// the wall roll and the base roll are separate - a wall chance of zero does not hold fire at the eagle
TEST_F(BotsTest, BotShootsTheEagleWithTheWallChanceAtZero)
{
	_gameConfig.botShootObstacleChance = 0.0;
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize}, ObstacleType::Eagle);

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(_allObjects.size(), before);
}

// shutting the base roll is what holds the fire
TEST_F(BotsTest, BotHoldsFireAtTheEagleWhenItsOwnChanceIsZero)
{
	_gameConfig.botShootFortressChance = 0.0;
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize}, ObstacleType::Eagle);

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_allObjects.size(), before);
}
