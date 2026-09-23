#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/LevelRotation.h"
#include "components/ObstacleSpawner.h"
#include "components/TankSpawner.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/managers/RespawnManager.h"
#include "entities/pawns/Tank.h"
#include "enums/Author.h"
#include "enums/GameMode.h"
#include "enums/RespawnGroup.h"
#include "gtest/gtest.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace
{
// a folder of empty .map files - the rotation reads names off the disk and never opens them
class MapsFolder final
{
	std::filesystem::path _path;

public:
	explicit MapsFolder(const std::string& name)
		: _path{std::filesystem::temp_directory_path() / ("bc-levels-" + name)}
	{
		std::filesystem::remove_all(_path);
		std::filesystem::create_directories(_path);
	}

	MapsFolder(const MapsFolder&) = delete;
	MapsFolder(MapsFolder&&) = delete;
	MapsFolder& operator=(const MapsFolder&) = delete;
	MapsFolder& operator=(MapsFolder&&) = delete;

	~MapsFolder() { std::filesystem::remove_all(_path); }

	void Add(const std::string& stem) const { std::ofstream{_path / (stem + ".map")} << "#\n"; }

	[[nodiscard]] std::string Folder() const { return _path.generic_string(); }

	[[nodiscard]] std::string PathOf(const std::string& stem) const
	{
		return (_path / (stem + ".map")).generic_string();
	}
};
}//namespace

// the campaign is the folder, read in name order
TEST(LevelRotationTest, TheNextLevelIsTheNextNameInTheFolder)
{
	const MapsFolder maps{"order"};
	maps.Add("level1");
	maps.Add("level2");
	const LevelRotation rotation{maps.Folder()};

	EXPECT_EQ(rotation.PathAfter(maps.PathOf("level1")), maps.PathOf("level2"));
}

// and the last one is followed by the first, so the prompt on the scoreboard always points somewhere
TEST(LevelRotationTest, TheLastLevelLoopsBackToTheFirst)
{
	const MapsFolder maps{"loop"};
	maps.Add("level1");
	maps.Add("level2");
	const LevelRotation rotation{maps.Folder()};

	EXPECT_EQ(rotation.PathAfter(maps.PathOf("level2")), maps.PathOf("level1"));
}

// a map the folder does not hold - the console can load one by name - starts the campaign over
TEST(LevelRotationTest, AMapOutsideTheFolderStartsTheCampaignOver)
{
	const MapsFolder maps{"stranger"};
	maps.Add("level1");
	maps.Add("level2");
	const LevelRotation rotation{maps.Folder()};

	EXPECT_EQ(rotation.PathAfter("Resources/Maps/handmade.map"), maps.PathOf("level1"));
}

// with nothing to rotate through the match stays on the map it is on
TEST(LevelRotationTest, AnEmptyFolderLeavesTheLevelAsItIs)
{
	const MapsFolder maps{"empty"};
	const LevelRotation rotation{maps.Folder()};

	EXPECT_EQ(rotation.PathAfter(maps.PathOf("level1")), maps.PathOf("level1"));
}

// a won map hands the player on to the next one: the tier its stars earned and the lives it has left
// ride along, while the enemies and their count are the new map's own
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
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager,
								 _tankSpawner);
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

	//NOTE: the world is swept by SpawnManager in the game and by hand here - what is left over would
	//answer the search for the tank the new level spawned
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

// lives are the other half of what travels: one is spent whenever a tank takes the field, so a level
// change spends the next of the same three, while a restart hands out three again
TEST_F(LevelProgressionTest, TheLivesLeftRideOnToTheNextLevelAndARestartGivesThemBack)
{
	const unsigned short afterFirstSpawn{_playerOneLives};

	_events->EmitEvent(NextLevelRequestedEvent{});
	StartMatch(GameResetEvent{.keepsPlayerProgress = true});

	EXPECT_LT(_playerOneLives, afterFirstSpawn) << "the new level handed out a fresh set of lives";

	StartMatch(GameResetEvent{});

	EXPECT_EQ(_playerOneLives, afterFirstSpawn);
}
