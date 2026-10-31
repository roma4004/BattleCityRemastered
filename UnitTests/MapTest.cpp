#include "application/GameConfig.h"
#include "application/Simulation.h"
#include "components/EventSystem.h"
#include "components/LevelRotation.h"
#include "components/Map.h"
#include "components/MapData.h"
#include "components/MapLoader.h"
#include "components/ObstacleSpawner.h"
#include "components/WorldGeometry.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/BaseObj.h"
#include "enums/BonusType.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/ObstacleType.h"
#include "enums/TankModel.h"
#include "utils/UuidUtils.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>
#include "TestUtils.h"//NOTE: PrintTo for the Point types

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

// the level: a character grid read into a map, the folder of levels it rotates through, and how a level starts
class MapTest : public testing::Test
{
protected:
	//NOTE: three rows of four, one of each interesting kind - enough to tell a parse from a guess
	static constexpr auto kTinyMap{"# a comment\n"
			"\n"
			"0123\n"
			"4567\n"
			"0000\n"};

	//NOTE: the smallest grid the rules pass - both edge gaps and the eagle's square are a tank wide
	static constexpr auto kPlayableGrid{"000000000000\n"
			"000000000000\n"
			"000000000000\n"
			"000000000000\n"
			"000030000000\n"
			"000000000000\n"
			"000000000000\n"
			"000000000000\n"};

	//NOTE: 12 characters and the newline - a row of the grid above
	static constexpr std::size_t kRowStride{13u};

	[[nodiscard]] static std::expected<void, MapError> ValidateGrid(const std::string& grid)
	{
		const auto map{MapLoader::Parse(grid)};
		if (!map)
		{
			return std::unexpected(map.error());
		}

		return MapLoader::Validate(*map);
	}

	//NOTE: swept whatever the assertions did, so a failed one leaves nothing on disk
	std::filesystem::path _tempMap{};

	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	GameConfig _gameConfig{};
	Simulation _simulation{_events, _gameConfig};
	//NOTE: what a level start emitted, in the order it did
	std::vector<std::string_view> _order{};
	std::vector<EventSubscription> _subs{};

	void SetUp() override
	{
		_subs.push_back(_events->AddListener([this](const GameResetEvent&) { _order.emplace_back("reset"); }));
		_subs.push_back(_events->AddListener([this](const LoadMapEvent&) { _order.emplace_back("map"); }));
		_subs.push_back(_events->AddListener([this](const AddToSpawnQueueEvent&)
		{
			if (_order.empty() || _order.back() != "spawn")
			{
				_order.emplace_back("spawn");
			}
		}));
	}

	void TearDown() override
	{
		std::error_code ec;
		std::filesystem::remove(_tempMap, ec);
	}
};

// a letter marks a bonus lying on the map: the cell under it stays free ground, a capital lays it golden
TEST_F(MapTest, ALetterLaysABonusOnFreeGround)
{
	for (const auto& [grid, isSuper]: {std::pair{"0h00\n0000\n", false}, std::pair{"0H00\n0000\n", true}})
	{
		SCOPED_TRACE(grid);
		const auto map{MapLoader::Parse(grid)};

		ASSERT_TRUE(map.has_value()) << map.error().reason;
		EXPECT_EQ(map->At(1u, 0u), ObstacleType::None);
		ASSERT_EQ(map->bonuses.size(), 1u);
		EXPECT_EQ(map->bonuses.front().col, 1u);
		EXPECT_EQ(map->bonuses.front().row, 0u);
		EXPECT_EQ(map->bonuses.front().type, BonusType::Helmet);
		EXPECT_EQ(map->bonuses.front().isSuper, isSuper);
	}
}

// "N enemies:" gives the level its count and the models of the first ones in order; no line, or a commented one, none
TEST_F(MapTest, TheEnemyLineGivesTheCountAndTheModels)
{
	struct Enemies final
	{
		std::string_view text;
		std::optional<std::size_t> count;
		std::vector<TankModel> lineup;
	};

	const std::array kEnemies{
			Enemies{.text = "6 enemies: 2 armor, fast, 1 basic\n0000\n",
					.count = 6u,
					.lineup = {TankModel::Armor, TankModel::Armor, TankModel::Fast, TankModel::Basic}},
			Enemies{.text = "7 enemies:\n0000\n", .count = 7u, .lineup = {}},
			Enemies{.text = kTinyMap, .count = std::nullopt, .lineup = {}},
			Enemies{.text = "# 3 enemies: armor\n0000\n", .count = std::nullopt, .lineup = {}}};
	for (const Enemies& enemies: kEnemies)
	{
		SCOPED_TRACE(enemies.text);
		const auto map{MapLoader::Parse(enemies.text)};

		ASSERT_TRUE(map.has_value()) << map.error().reason;
		EXPECT_EQ(map->enemyCount, enemies.count);
		EXPECT_EQ(map->enemyLineup, enemies.lineup);
	}
}

// the rules of the game on top of the grid: one eagle, and a gap a tank wide at both ends - bush is ground, not wall
TEST_F(MapTest, AMatchNeedsOneEagleAndAWayInAtBothEnds)
{
	struct Field final
	{
		std::string_view why;
		std::string grid;
		bool isPlayable{};
	};

	const auto withRow = [](const std::size_t row, const std::string& cells)
	{
		std::string grid{kPlayableGrid};
		grid.replace(kRowStride * row, cells.size(), cells);
		return grid;
	};
	std::string noEagle{kPlayableGrid};
	noEagle[noEagle.find('3')] = '0';
	std::string twoEagles{kPlayableGrid};
	twoEagles[twoEagles.find('0')] = '3';
	const std::string wall(kRowStride - 1u, '1');
	std::string narrowGap{wall};
	narrowGap.replace(4u, 2u, "00");

	//NOTE: the eagle is one character but a tank wide, its body zeros in the file - no spawn spot inside the fortress
	const std::array fields{Field{.why = "the smallest playable grid", .grid = kPlayableGrid, .isPlayable = true},
							Field{.why = "no eagle", .grid = noEagle},
							Field{.why = "a second eagle", .grid = twoEagles},
							Field{.why = "a bush edge", .grid = withRow(0u, std::string(kRowStride - 1u, '6')),
								  .isPlayable = true},
							Field{.why = "the top edge sealed", .grid = withRow(0u, wall)},
							Field{.why = "the bottom edge sealed", .grid = withRow(7u, wall)},
							Field{.why = "a gap narrower than a tank", .grid = withRow(0u, narrowGap)},
							Field{.why = "the eagle's own body", .grid = "00000\n00000\n00000\n03000\n00000\n00000\n"}};
	for (const Field& field: fields)
	{
		SCOPED_TRACE(field.why);

		EXPECT_EQ(ValidateGrid(field.grid).has_value(), field.isPlayable);
	}
}

//NOTE: the client sizes an eagle from a position-only spawn event and used to give it a single cell
TEST_F(MapTest, ClientGivesTheEagleTheSameSpanTheMapDoes)
{
	const auto events{std::make_shared<EventSystem>()};
	std::vector<std::shared_ptr<BaseObj>> allObjects;
	auto spawnQueueSub{TestUtils::WireSpawnQueue(events, allObjects)};

	GameConfig gameConfig{};
	gameConfig.gameMode = GameMode::PlayAsClient;
	const ObstacleSpawner spawner{events, gameConfig};

	const double cell{gameConfig.gridOffset};
	events->EmitEvent(ObstacleSpawnedEvent{.pos = {.x = 0.0, .y = 0.0},
										   .type = ObstacleType::Eagle,
										   .uuid = UuidUtils::GetRandomUuid()});
	events->EmitEvent(ObstacleSpawnedEvent{.pos = {.x = 0.0, .y = 0.0},
										   .type = ObstacleType::Brick,
										   .uuid = UuidUtils::GetRandomUuid()});

	ASSERT_EQ(allObjects.size(), 2u);
	EXPECT_DOUBLE_EQ(allObjects[0]->GetRect().w, cell * ObstacleCellSpan(ObstacleType::Eagle));
	EXPECT_DOUBLE_EQ(allObjects[1]->GetRect().w, cell);
}

// a baseless map leaves out the eagle, its wall and the shovel
TEST_F(MapTest, ABaselessMatchLeavesTheEagleItsWallAndTheShovelOut)
{
	_tempMap = std::filesystem::temp_directory_path()
			   / ("battlecity_base_" + UuidUtils::GetStringUuid(UuidUtils::GetRandomUuid()) + ".map");
	std::string grid{kPlayableGrid};
	grid[kRowStride * 3u + 4u] = '4';
	grid[kRowStride * 1u + 1u] = 'v';
	grid[kRowStride * 1u + 2u] = 'h';
	std::ofstream{_tempMap} << grid;

	const auto events{std::make_shared<EventSystem>()};
	std::vector<ObstacleType> obstacles{};
	std::vector<BonusType> bonuses{};
	const EventSubscription obstacleSub{events->AddListener([&obstacles](const SpawnObstacleEvent& event)
	{
		obstacles.push_back(event.type);
	})};
	const EventSubscription bonusSub{events->AddListener([&bonuses](const SpawnMapBonusEvent& event)
	{
		bonuses.push_back(event.type);
	})};
	Map map{events};
	ASSERT_TRUE(map.LoadFromFile(_tempMap).has_value());

	map.CreateObstacles(WorldGeometry::kCellSize, true);
	map.CreateBonuses(WorldGeometry::kCellSize, WorldGeometry::kCellSize, true);

	EXPECT_FALSE(std::ranges::contains(obstacles, ObstacleType::Eagle));
	EXPECT_FALSE(std::ranges::contains(obstacles, ObstacleType::Fortress));
	EXPECT_EQ(bonuses, std::vector{BonusType::Helmet});
}

// the campaign is the folder, read in name order
TEST_F(MapTest, TheNextLevelIsTheNextNameInTheFolder)
{
	const MapsFolder maps{"order"};
	maps.Add("level1");
	maps.Add("level2");
	const LevelRotation rotation{maps.Folder()};

	EXPECT_EQ(rotation.PathAfter(maps.PathOf("level1")), maps.PathOf("level2"));
}

// and the last one is followed by the first, so the prompt on the scoreboard always points somewhere
TEST_F(MapTest, TheLastLevelLoopsBackToTheFirst)
{
	const MapsFolder maps{"loop"};
	maps.Add("level1");
	maps.Add("level2");
	const LevelRotation rotation{maps.Folder()};

	EXPECT_EQ(rotation.PathAfter(maps.PathOf("level2")), maps.PathOf("level1"));
}

// a map the folder does not hold - the console can load one by name - starts the campaign over
TEST_F(MapTest, AMapOutsideTheFolderStartsTheCampaignOver)
{
	const MapsFolder maps{"stranger"};
	maps.Add("level1");
	maps.Add("level2");
	const LevelRotation rotation{maps.Folder()};

	EXPECT_EQ(rotation.PathAfter("Resources/Maps/handmade.map"), maps.PathOf("level1"));
}

// with nothing to rotate through the match stays on the map it is on
TEST_F(MapTest, AnEmptyFolderLeavesTheLevelAsItIs)
{
	const MapsFolder maps{"empty"};
	const LevelRotation rotation{maps.Folder()};

	EXPECT_EQ(rotation.PathAfter(maps.PathOf("level1")), maps.PathOf("level1"));
}

//NOTE: the reset and the map load used to hang off MatchStartedEvent, ordered only by who subscribed first
TEST_F(MapTest, TheMapFillsTheFieldTheResetJustEmptied)
{
	//NOTE: the spawners are built on the mode change, so this is what gives the match a map to load
	_simulation.ApplyGameMode(GameMode::OnePlayer);
	_order.clear();

	_events->EmitEvent(MatchStartedEvent{});

	EXPECT_EQ(_order, (std::vector<std::string_view>{"reset", "map", "spawn"}));
}

// one reset, then the next level's map - a reset after the load empties the level, a second one tears it down twice
TEST_F(MapTest, TheNextLevelIsOneResetThenItsMap)
{
	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	_order.clear();
	_events->EmitEvent(GameFinishedEvent{.state = GameState::Won});
	_events->EmitEvent(NextLevelRequestedEvent{});
	_events->EmitEvent(PostTickUpdateEvent{});

	EXPECT_EQ(_order, (std::vector<std::string_view>{"reset", "map", "spawn"}));
}

// a restart is a new run, not one more level of the old one
TEST_F(MapTest, ARestartStartsOverFromTheFirstStage)
{
	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	_order.clear();
	_events->EmitEvent(GameFinishedEvent{.state = GameState::Won});
	_events->EmitEvent(NextLevelRequestedEvent{});
	_events->EmitEvent(PostTickUpdateEvent{});
	ASSERT_EQ(_gameConfig.stageNumber, 2u) << "the control failed - the next level did not count";

	_events->EmitEvent(MatchRestartRequestedEvent{});

	EXPECT_EQ(_gameConfig.stageNumber, 1u);
}

// the ask is answered a step later, so every listener of it still sees the field it has to read
TEST_F(MapTest, TheFieldStandsUntilTheFrameEnds)
{
	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	_order.clear();
	_events->EmitEvent(GameFinishedEvent{.state = GameState::Won});
	_events->EmitEvent(NextLevelRequestedEvent{});

	EXPECT_TRUE(_order.empty()) << "the world was torn down inside the ask for the next level";
}
