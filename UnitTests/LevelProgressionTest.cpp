#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/ObstacleSpawner.h"
#include "components/TankSpawner.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/managers/RespawnManager.h"
#include "entities/pawns/Tank.h"
#include "enums/Author.h"
#include "enums/GameMode.h"
#include "enums/RespawnGroup.h"
#include "geometry/ObjRectangle.h"
#include "gtest/gtest.h"
#include "utils/Uuid.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// a won map hands the player on: tier and lives left ride along, the enemies and their count are the new map's
class LevelProgressionTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	std::vector<EventSubscription> _subs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	EventSubscription _spawnQueueSub{};
	unsigned short _playerOneLives{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_allObjects.reserve(6u);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		ApplyGameMode(GameMode::OnePlayer);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);

		_subs.push_back(_events->AddListener([this](const RespawnCountChangedToEvent& event)
		{
			if (event.group == RespawnGroup::PLAYER1)
			{
				_playerOneLives = event.respawnCount;
			}
		}));

		StartMatch(GameResetEvent{});
	}

	void ApplyGameMode(const GameMode mode)
	{
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, mode, _respawnManager, _tankSpawner);
	}

	//NOTE: swept by hand here, by SpawnManager in the game - leftovers would answer the search for the new tank
	void StartMatch(const GameResetEvent& reset)
	{
		_events->EmitEvent(reset);
		_allObjects.clear();
		_events->EmitEvent(RespawnTanksEvent{});
	}

	[[nodiscard]] std::shared_ptr<Tank> PlayerOne() const
	{
		for (const std::shared_ptr<BaseObj>& obj: _allObjects)
		{
			const auto tank{std::dynamic_pointer_cast<Tank>(obj)};
			if (tank != nullptr && tank->GetAuthor() == Author::Player1)
			{
				return tank;
			}
		}

		return nullptr;
	}
};

// the control: a star raises the tier, and an ordinary restart hands the player a fresh tank
TEST_F(LevelProgressionTest, ARestartTakesTheTierAway)
{
	ASSERT_NE(PlayerOne(), nullptr);
	_events->EmitEvent(Key(Author::Player1), BonusStarPickupEvent{});
	ASSERT_EQ(PlayerOne()->GetTier(), 2u);

	StartMatch(GameResetEvent{});

	ASSERT_NE(PlayerOne(), nullptr);
	EXPECT_EQ(PlayerOne()->GetTier(), 1u);
}

// the same tier survives a level change, because the request is answered while the won field still stands
TEST_F(LevelProgressionTest, TheTierRidesOnToTheNextLevel)
{
	_events->EmitEvent(Key(Author::Player1), BonusStarPickupEvent{});
	ASSERT_EQ(PlayerOne()->GetTier(), 2u);

	_events->EmitEvent(NextLevelRequestedEvent{});
	StartMatch(GameResetEvent{.keepsPlayerProgress = true});

	ASSERT_NE(PlayerOne(), nullptr);
	EXPECT_EQ(PlayerOne()->GetTier(), 2u);
}

// the tank lands as big as the tier it carries makes it, and its spot is cut to that - all of it on the field
TEST_F(LevelProgressionTest, ATankCarryingATierLandsGrownAndWhollyOnTheField)
{
	const double ungrown{PlayerOne()->GetRect().w};
	_events->EmitEvent(Key(Author::Player1), BonusStarPickupEvent{});
	_events->EmitEvent(Key(Author::Player1), BonusStarPickupEvent{});
	_events->EmitEvent(Key(Author::Player1), BonusStarPickupEvent{});
	_events->EmitEvent(NextLevelRequestedEvent{});
	StartMatch(GameResetEvent{.keepsPlayerProgress = true});

	ASSERT_NE(PlayerOne(), nullptr);
	const ObjRectangle hull{PlayerOne()->GetRect()};
	EXPECT_GT(hull.w, ungrown);
	EXPECT_LE(hull.Bottom(), static_cast<double>(_gameConfig.battlefieldSize.y));
}

// it rides once: dying on the new level costs the tier, the way it always did
TEST_F(LevelProgressionTest, TheLoadoutIsSpentOnTheFirstSpawnOfTheLevel)
{
	_events->EmitEvent(Key(Author::Player1), BonusStarPickupEvent{});
	_events->EmitEvent(NextLevelRequestedEvent{});
	StartMatch(GameResetEvent{.keepsPlayerProgress = true});

	const Uuid uuid{PlayerOne()->GetUuid()};
	_events->EmitEvent(TankDiedEvent{.who = Author::Player1, .uuid = uuid, .author = Author::Enemy1});
	_allObjects.clear();
	_events->EmitEvent(RespawnTanksEvent{});

	ASSERT_NE(PlayerOne(), nullptr);
	EXPECT_EQ(PlayerOne()->GetTier(), 1u);
}

// the health rides along as well - whatever the bonuses healed above the spawn health is kept
TEST_F(LevelProgressionTest, TheHealthRidesOnToTheNextLevel)
{
	const unsigned int spawnHealth{PlayerOne()->GetHealth()};
	PlayerOne()->Heal(1);

	_events->EmitEvent(NextLevelRequestedEvent{});
	StartMatch(GameResetEvent{.keepsPlayerProgress = true});

	ASSERT_NE(PlayerOne(), nullptr);
	EXPECT_GT(PlayerOne()->GetHealth(), spawnHealth);
}

// but never less than that: a wounded tank starts the next level whole
TEST_F(LevelProgressionTest, AWoundedTankStartsTheNextLevelWhole)
{
	const unsigned int spawnHealth{PlayerOne()->GetHealth()};
	PlayerOne()->TakeDamage(1u, Author::Enemy1);

	_events->EmitEvent(NextLevelRequestedEvent{});
	StartMatch(GameResetEvent{.keepsPlayerProgress = true});

	ASSERT_NE(PlayerOne(), nullptr);
	EXPECT_EQ(PlayerOne()->GetHealth(), spawnHealth);
}

// a client builds the tank from its model alone, so the host reports the health and the tier the tank brought along
TEST_F(LevelProgressionTest, TheHostReportsWhatTheTankCarried)
{
	ApplyGameMode(GameMode::PlayAsHost);
	StartMatch(GameResetEvent{});
	PlayerOne()->Heal(1);
	_events->EmitEvent(Key(Author::Player1), BonusStarPickupEvent{});
	const unsigned int health{PlayerOne()->GetHealth()};
	const unsigned int tier{PlayerOne()->GetTier()};
	std::vector<HealthChangedEvent> healths;
	std::vector<TierChangedEvent> tiers;
	const EventSubscription healthSub{_events->AddListener([&healths](const HealthChangedEvent& event)
	{
		healths.push_back(event);
	})};
	const EventSubscription tierSub{_events->AddListener([&tiers](const TierChangedEvent& event)
	{
		tiers.push_back(event);
	})};

	_events->EmitEvent(NextLevelRequestedEvent{});
	StartMatch(GameResetEvent{.keepsPlayerProgress = true});

	const auto healthReport{std::ranges::find(healths, PlayerOne()->GetUuid(), &HealthChangedEvent::uuid)};
	ASSERT_NE(healthReport, healths.end());
	EXPECT_EQ(healthReport->health, health);
	const auto tierReport{std::ranges::find(tiers, PlayerOne()->GetUuid(), &TierChangedEvent::uuid)};
	ASSERT_NE(tierReport, tiers.end());
	EXPECT_EQ(tierReport->tier, tier);
}

// lives travel too: the ones lost stay lost, and the tank that saw the level out takes the field again for free
TEST_F(LevelProgressionTest, TheLivesLeftRideOnToTheNextLevel)
{
	_events->EmitEvent(TankDiedEvent{.who = Author::Player1, .uuid = PlayerOne()->GetUuid(), .author = Author::Enemy1});
	_allObjects.clear();
	_events->EmitEvent(RespawnTanksEvent{});
	const unsigned short beforeTheWin{_playerOneLives};

	_events->EmitEvent(NextLevelRequestedEvent{});
	StartMatch(GameResetEvent{.keepsPlayerProgress = true});

	EXPECT_EQ(_playerOneLives, beforeTheWin);
}

// while a restart hands out three again, whatever the run spent
TEST_F(LevelProgressionTest, ARestartGivesTheLivesBack)
{
	const unsigned short afterFirstSpawn{_playerOneLives};
	_events->EmitEvent(TankDiedEvent{.who = Author::Player1, .uuid = PlayerOne()->GetUuid(), .author = Author::Enemy1});
	_allObjects.clear();
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(NextLevelRequestedEvent{});
	StartMatch(GameResetEvent{.keepsPlayerProgress = true});
	ASSERT_LT(_playerOneLives, afterFirstSpawn);

	StartMatch(GameResetEvent{});

	EXPECT_EQ(_playerOneLives, afterFirstSpawn);
}
