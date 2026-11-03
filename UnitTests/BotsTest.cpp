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
#include <cstddef>
#include <memory>
#include <string_view>
#include <tuple>
#include <vector>

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
	static constexpr double kDeltaTimeOneFrame{1.0 / 60.0};
	double _tankSize{};
	double _gridSize{};
	static constexpr unsigned short kTankHealth{100u};
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

	[[nodiscard]] std::shared_ptr<Tank> CreateBot(const FPoint pos, const Author author, const Direction dir,
												  const unsigned short tier = 1u) const
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};

		return TestUtils::CreateBot(rect, kTankHealth, author, _allObjects, _events, dir, _tankPool, _gameConfig, tier);
	}

	[[nodiscard]] std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author, const Direction dir) const
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};

		return TestUtils::CreatePlayer(rect, kTankHealth, author, _allObjects, _events, dir, _tankPool, _gameConfig);
	}

	[[nodiscard]] std::shared_ptr<Bullet> CreateBullet(const FPoint pos, const Direction dir, const Author author,
													   const BulletCaliber& caliber) const
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = caliber.size.x, .h = caliber.size.y};

		return TestUtils::CreateBullet(rect, _bulletPool, _events, caliber, dir, author);
	}

	void SpawnObstacleArea(const ObjRectangle& area, const ObstacleType type) const
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

	[[nodiscard]] std::shared_ptr<BaseObj> SpawnObstacle(const ObjRectangle& rect, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, rect, type);
	}
};

// a bot turns onto an opponent it sees down the row
TEST_F(BotsTest, BotsChangeDirectionIfOpponentSeen)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
	std::ignore = CreateBot({.x = _tankSize * 3.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::RIGHT);
}

// the second bot decides once the shot exists and answers it, not its shooter - the top edge leaves only down
TEST_F(BotsTest, ABotAnswersTheShotOnItsWayRatherThanTheShooter)
{
	std::ignore = CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);
	const auto enemyBot{CreateBot({.x = _tankSize * 3.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetDirection(), Direction::DOWN);
}

// an ally in sight is no reason to turn the hull, a player or a bot alike
TEST_F(BotsTest, BotsNoChangeDirectionIfAllySeen)
{
	for (const bool isAllyABot: {false, true})
	{
		SCOPED_TRACE(isAllyABot);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
		const FPoint allyPos{.x = _tankSize * 3.0, .y = 0.0};
		if (isAllyABot)
		{
			std::ignore = CreateBot(allyPos, Author::Player2, Direction::DOWN);
		}
		else
		{
			std::ignore = CreatePlayer(allyPos, Author::Player2, Direction::DOWN);
		}

		_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

		EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN);
	}
}

// A bot does not turn to an opponent it cannot shoot without catching its own blast
TEST_F(BotsTest, BotsNoChangeDirectionIfOpponentTooClose)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	const auto enemyBot{CreateBot({.x = _tankSize, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

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

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});
	ASSERT_EQ(bot->GetDirection(), Direction::UP) << "it never lined up on the tank across the water";

	target->SetIsAlive(false);

	//NOTE: two ticks - the sights are read at the end of one, so the hull is freed on the next
	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	EXPECT_NE(bot->GetDirection(), Direction::UP) << "it kept the heading of a tank that is no longer there";
}

// a bonus in sight, on the other hand, turns the bot towards it
TEST_F(BotsTest, BotsChangeDirectionIfBonusSeenAndNoOneShoot)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 21.0, .y = 0.0, .w = _tankSize, .h = _tankSize});

	const auto enemyBot{CreateBot({.x = _tankSize * 3.0 + 40.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	const Direction endDirCoop{coopBot->GetDirection()};
	const Direction endDirEnemy{enemyBot->GetDirection()};

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::LEFT);
}

// but not while an opponent it can shoot stands in the sights - the bonus can wait, the shot cannot
TEST_F(BotsTest, ABonusAtTheSideDoesNotTurnABotOffAnOpponentItCanShoot)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	std::ignore = CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Enemy1, Direction::DOWN);

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 21.0, .y = 0.0, .w = _tankSize, .h = _tankSize});

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN) << "it turned off a loaded shot for the bonus";
}

// and once the shot is away the bonus is the thing to do with the reload
TEST_F(BotsTest, AReloadingBotGoesForTheBonusItPassedUp)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	std::ignore = CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Enemy1, Direction::DOWN);

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 21.0, .y = 0.0, .w = _tankSize, .h = _tankSize});

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

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
		std::ignore = SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
									terrain);
		if (isBonusOnIt)
		{
			std::ignore = SpawnObstacle(
					ObjRectangle{.x = 0.0, .y = _tankSize * 3.0 + 1.0, .w = _tankSize, .h = _tankSize}, terrain);
		}

		_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

		_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

		EXPECT_EQ(coopBot->GetDirection(), turnedTo);
	}
}

// a bonus off the line it drives is no reason to turn
TEST_F(BotsTest, BotsNoChangeDirectionIfBonusOutsideLineOfSight)
{
	const auto bot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
	_bonusSpawner->SpawnRandomBonus({.x = _tankSize * 2.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize});

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	EXPECT_EQ(bot->GetDirection(), Direction::DOWN);
}

// a shot coming down our line is answered with a shot, hull kept - turning to face a bullet is driving at it
TEST_F(BotsTest, BotShootsDownAnIncomingBulletWithoutTurning)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::RIGHT)};

	const double bulletHeight{coopBot->GetBulletHeight()};

	// head-on: to the right of the bot, flying at it, and far enough that a shot still meets it
	std::ignore = CreateBullet({.x = _tankSize * 4.0, .y = (_tankSize - bulletHeight) / 2.0}, Direction::LEFT,
							   Author::Enemy1, IncomingCaliber(*coopBot));

	const std::size_t worldSizeBeforeShot{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::RIGHT) << "the hull turned towards the bullet";
	EXPECT_GT(_allObjects.size(), worldSizeBeforeShot) << "nothing was fired at it";
}

// a bullet that never reaches the bot is somebody else's problem - one flying away or one a row over
TEST_F(BotsTest, BotIgnoresABulletThatNeverReachesIt)
{
	struct Shot final
	{
		std::string_view why;
		Direction facing{};
		FPoint from{};
		Direction heading{};
	};

	//NOTE: facing along the bullet's axis when it flies away - a dodge goes across it, so a false alarm shows as a turn
	for (const Shot& shot: {Shot{.why = "flying away",
								 .facing = Direction::RIGHT,
								 .from = {.x = _tankSize * 2.0, .y = _tankSize * 3.0},
								 .heading = Direction::RIGHT},
							Shot{.why = "two rows up, flying past",
								 .facing = Direction::DOWN,
								 .from = {.x = _tankSize * 4.0, .y = _tankSize},
								 .heading = Direction::LEFT}})
	{
		SCOPED_TRACE(shot.why);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto coopBot{CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, shot.facing)};
		const double bulletHeight{coopBot->GetBulletHeight()};
		std::ignore = CreateBullet({.x = shot.from.x, .y = shot.from.y + (_tankSize - bulletHeight) / 2.0},
								   shot.heading, Author::Enemy1, IncomingCaliber(*coopBot));

		_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

		EXPECT_EQ(coopBot->GetDirection(), shot.facing) << "a bullet that never reaches it was dodged";
	}
}

// a bullet crossing our lane is nothing to shoot - the answer is to leave the line across its path
TEST_F(BotsTest, BotStepsOutOfTheLaneOfACrossingBullet)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::DOWN)};

	const double bulletHeight{coopBot->GetBulletHeight()};
	// to the right of the bot and flying left, so it arrives across the way the bot is looking
	std::ignore = CreateBullet({.x = _tankSize * 4.0, .y = _tankSize * 3.0 + (_tankSize - bulletHeight) / 2.0},
							   Direction::LEFT, Author::Enemy1, IncomingCaliber(*coopBot));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	const Direction dodged{coopBot->GetDirection()};

	EXPECT_TRUE(dodged == Direction::UP || dodged == Direction::DOWN)
			<< "the dodge went along the bullet's lane instead of out of it";
}

// a dodge goes to the side with room and has to end outside the lane - with no room for that the hull stays put
TEST_F(BotsTest, BotDodgesOnlyWhereThereIsRoomToLeaveTheLane)
{
	struct Lane final
	{
		std::string_view why;
		FPoint bot{};
		Direction facing{};
		std::vector<ObjRectangle> walls;
		double bulletX{};
		Direction answer{};
	};

	const double gap{_gridSize / 2.0};
	const double laneY{_gridSize * 5.0 + gap};
	//NOTE: in the narrow passage the bullet is too close to shoot down too, or the intercept would hold the hull
	for (const Lane& lane: {Lane{.why = "a wall right above leaves only downwards",
								 .bot = {.x = _tankSize * 4.0, .y = _tankSize * 3.0},
								 .facing = Direction::DOWN,
								 .walls = {{.x = _tankSize * 4.0,
											.y = _tankSize * 2.0,
											.w = _tankSize,
											.h = _tankSize}},
								 .bulletX = _tankSize * 8.0,
								 .answer = Direction::DOWN},
							Lane{.why = "a passage too narrow to leave the lane by",
								 .bot = {.x = _tankSize * 3.0, .y = laneY},
								 .facing = Direction::RIGHT,
								 .walls = {{.x = 0.0, .y = _gridSize * 4.0, .w = _tankSize * 8.0, .h = _gridSize},
										   {.x = 0.0,
											.y = laneY + _tankSize + gap,
											.w = _tankSize * 8.0,
											.h = _gridSize}},
								 .bulletX = _tankSize * 4.0 + _gridSize * 2.0,
								 .answer = Direction::RIGHT}})
	{
		SCOPED_TRACE(lane.why);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto coopBot{CreateBot(lane.bot, Author::Player1, lane.facing)};
		for (const ObjRectangle& wall: lane.walls)
		{
			SpawnObstacleArea(wall, ObstacleType::Steel);
		}
		const double bulletHeight{coopBot->GetBulletHeight()};
		std::ignore = CreateBullet({.x = lane.bulletX, .y = lane.bot.y + (_tankSize - bulletHeight) / 2.0},
								   Direction::LEFT, Author::Enemy1, IncomingCaliber(*coopBot));

		_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

		EXPECT_EQ(coopBot->GetDirection(), lane.answer);
	}
}

// reloading suppresses aiming too, so with a shot coming back the bot's one answer is to leave the line
TEST_F(BotsTest, AReloadingBotStepsAsideFromTheShotComingBack)
{
	const auto bot{CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::RIGHT)};
	std::ignore = CreateBot({.x = _tankSize * 3.0, .y = _tankSize * 3.0}, Author::Enemy1, Direction::LEFT);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});
	ASSERT_EQ(bot->GetDirection(), Direction::RIGHT) << "the first frame is the exchange of shots";

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	const Direction dodged{bot->GetDirection()};

	EXPECT_TRUE(dodged == Direction::UP || dodged == Direction::DOWN)
			<< "a reloading bot stood in the lane of the answer";
}

// whom a bot fires at is decided by sides; the wall rolls are shut, and only the shooter's shots are counted
TEST_F(BotsTest, ABotShootsAnOpponentDownTheLine)
{
	struct Duel final
	{
		GameMode mode{};
		Author shooter{};
		Author target{};
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
		std::ignore = CreateBot({.x = 0.0, .y = 0.0}, shooter, Direction::DOWN);
		const FPoint targetPos{.x = 0.0, .y = _tankSize * 3.0};
		if (isTargetABot)
		{
			std::ignore = CreateBot(targetPos, target, Direction::DOWN);
		}
		else
		{
			std::ignore = CreatePlayer(targetPos, target, Direction::UP);
		}

		_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

		const auto shots{std::ranges::count_if(_allObjects, [shooter](const std::shared_ptr<BaseObj>& object)
		{
			const auto* bullet{dynamic_cast<const Bullet*>(object.get())};

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
		Author shooter{};
		Author ally{};
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
		std::ignore = CreateBot({.x = 0.0, .y = 0.0}, shooter, Direction::DOWN);
		const FPoint allyPos{.x = 0.0, .y = _tankSize * 3.0};
		if (isAllyABot)
		{
			std::ignore = CreateBot(allyPos, ally, Direction::DOWN);
		}
		else
		{
			std::ignore = CreatePlayer(allyPos, ally, Direction::UP);
		}

		const std::size_t before{_allObjects.size()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

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
		std::ignore = CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
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
			std::ignore = CreateBot(behind, Author::Enemy2, Direction::DOWN);
		}

		_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

		const auto shots{std::ranges::count_if(_allObjects, [](const std::shared_ptr<BaseObj>& object)
		{
			const auto* const bullet{dynamic_cast<const Bullet*>(object.get())};

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
		Author shooter{};
		ObstacleType part{};
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
		std::ignore = CreateBot({.x = 0.0, .y = 0.0}, shooter, Direction::DOWN);
		SpawnObstacleArea({.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize}, part);

		const std::size_t before{_allObjects.size()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

		EXPECT_EQ(_allObjects.size(), before + (isShot ? 1u : 0u));
	}
}

// water and ice hide nobody, a bush hides a player behind it or inside it, and a wall between them breaks the sight
TEST_F(BotsTest, BotShootsAPlayerOnlyInSight)
{
	struct Sight final
	{
		ObstacleType between{};
		bool isInside{};
		bool isShot{};
	};

	//NOTE: the rolls shut - the bot fires at a player in sight and at nothing else
	_gameConfig.botShootObstacleChance = 0.0;
	_gameConfig.botShootFortressChance = 0.0;
	for (const Sight& sight: {Sight{.between = ObstacleType::Water, .isShot = true},
							  Sight{.between = ObstacleType::Water, .isInside = true, .isShot = true},
							  Sight{.between = ObstacleType::Ice, .isShot = true},
							  Sight{.between = ObstacleType::Ice, .isInside = true, .isShot = true},
							  Sight{.between = ObstacleType::Bush},
							  Sight{.between = ObstacleType::Bush, .isInside = true},
							  Sight{.between = ObstacleType::Brick},
							  Sight{.between = ObstacleType::Steel},
							  Sight{.between = ObstacleType::Fortress}})
	{
		SCOPED_TRACE(testing::Message() << static_cast<int>(sight.between) << (sight.isInside ? " inside" : " behind"));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		std::ignore = CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
		const double obstacleY{sight.isInside ? _tankSize * 3.0 + 1.0 : _tankSize * 2.0 + 1.0};
		SpawnObstacleArea({.x = 0.0, .y = obstacleY, .w = _tankSize, .h = _tankSize}, sight.between);
		const double playerY{sight.isInside ? _tankSize * 3.0 + 1.0 : _tankSize * 3.0 + 2.0};
		std::ignore = CreatePlayer({.x = 0.0, .y = playerY}, Author::Player1, Direction::UP);

		const std::size_t before{_allObjects.size()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

		EXPECT_EQ(_allObjects.size(), before + (sight.isShot ? 1u : 0u));
	}
}

// one shot per cooldown however long the target stays in sight - the gate is in Tank::TickUpdate
TEST_F(BotsTest, BotDoesNotShootTwiceWithinOneCooldown)
{
	std::ignore = CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::RIGHT);
	std::ignore = CreateBot({.x = _tankSize * 3.0, .y = 0.0}, Author::Enemy1, Direction::LEFT);

	const std::size_t beforeFirstShot{_allObjects.size()};
	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});
	const std::size_t afterFirstShot{_allObjects.size()};
	ASSERT_GT(afterFirstShot, beforeFirstShot);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	EXPECT_EQ(_allObjects.size(), afterFirstShot);
}

// no fire at a brick a muzzle away, inside its own blast - water on the other sides, so no turn finds another target
TEST_F(BotsTest, BotHoldsFireWhenTheBlastWouldReachItself)
{
	const auto bot{CreateBot({.x = _tankSize, .y = _tankSize}, Author::Enemy1, Direction::RIGHT)};

	SpawnObstacleArea({.x = _tankSize * 2.0 + 3.0, .y = _tankSize, .w = _tankSize, .h = _tankSize},
					  ObstacleType::Brick);
	const std::array waters{ObjRectangle{.x = 0.0, .y = _tankSize, .w = _tankSize, .h = _tankSize},
							ObjRectangle{.x = _tankSize, .y = 0.0, .w = _tankSize, .h = _tankSize},
							ObjRectangle{.x = _tankSize, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize}};
	for (const ObjRectangle& waterRect: waters)
	{
		std::ignore = SpawnObstacle(waterRect, ObstacleType::Water);
	}

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	EXPECT_EQ(bot->GetDirection(), Direction::RIGHT);
	EXPECT_EQ(_allObjects.size(), before);
}

// terrain is no target with both rolls open: water, a bush and ice draw no fire
TEST_F(BotsTest, BotHoldsFireAtWaterBushAndIce)
{
	for (const ObstacleType terrain: {ObstacleType::Water, ObstacleType::Bush, ObstacleType::Ice})
	{
		SCOPED_TRACE(static_cast<int>(terrain));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		std::ignore = CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
		SpawnObstacleArea({.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize}, terrain);

		const std::size_t before{_allObjects.size()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

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

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	EXPECT_NE(bot->GetDirection(), Direction::RIGHT);
}

// steel draws fire only from the tier that breaks it - a cell farther off, the bot holds fire inside its blast
TEST_F(BotsTest, ABotShootsSteelOnlyFromTheTierThatBreaksIt)
{
	struct Gun final
	{
		unsigned short tier{};
		bool isShot{};
	};

	for (const auto& [tier, isShot]: {Gun{.tier = 1u}, Gun{.tier = 3u, .isShot = true}})
	{
		SCOPED_TRACE(tier);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		std::ignore = CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN, tier);
		SpawnObstacleArea({.x = 0.0, .y = _tankSize * 3.0 + _gridSize, .w = _tankSize, .h = _tankSize},
						  ObstacleType::Steel);

		const std::size_t before{_allObjects.size()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

		EXPECT_EQ(_allObjects.size(), before + (isShot ? 1u : 0u));
	}
}

// a wall in sight draws fire by the obstacle roll: at zero never, the roll being taken once per sighting; at one always
TEST_F(BotsTest, BotShootsAWallByTheObstacleRoll)
{
	for (const double chance: {0.0, 1.0})
	{
		SCOPED_TRACE(chance);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		_gameConfig.botShootObstacleChance = chance;
		const auto bot{CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};
		SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize},
						  ObstacleType::Brick);

		const std::size_t before{_allObjects.size()};

		for (int frame{}; frame < 10; ++frame)
		{
			_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});
		}

		ASSERT_EQ(bot->GetDirection(), Direction::DOWN) << "the control failed - the bot looked away from the wall";
		EXPECT_EQ(_allObjects.size() > before, chance > 0.0);
	}
}

// a bot flush against a wall would catch its own blast - the way out of a dead end is the turn, never the gun
TEST_F(BotsTest, ABlockedBotStillTurnsAwayAtZeroChance)
{
	_gameConfig.botShootObstacleChance = 0.0;

	const auto bot{CreateBot({.x = _tankSize * 2.0, .y = _tankSize * 2.0}, Author::Enemy1, Direction::RIGHT)};

	// flush against the bot's right side, destructible, and still not worth a shot from this close
	std::ignore = SpawnObstacle(
			ObjRectangle{.x = _tankSize * 3.0 + 1.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize},
			ObstacleType::Brick);

	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

	EXPECT_NE(bot->GetDirection(), Direction::RIGHT);
	EXPECT_EQ(_allObjects.size(), before);
}

// the wall roll and the base roll are separate - the eagle draws fire by its own roll, whatever the wall chance
TEST_F(BotsTest, BotShootsTheEagleByItsOwnRoll)
{
	struct Rolls final
	{
		double wall{};
		double base{};
	};

	for (const Rolls& rolls: {Rolls{.wall = 0.0, .base = 1.0}, Rolls{.wall = 1.0, .base = 0.0}})
	{
		SCOPED_TRACE(testing::Message() << "wall " << rolls.wall << ", base " << rolls.base);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		_gameConfig.botShootObstacleChance = rolls.wall;
		_gameConfig.botShootFortressChance = rolls.base;
		std::ignore = CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
		std::ignore = SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize},
									ObstacleType::Eagle);

		const std::size_t before{_allObjects.size()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = kDeltaTimeOneFrame});

		EXPECT_EQ(_allObjects.size() > before, rolls.base > 0.0);
	}
}
