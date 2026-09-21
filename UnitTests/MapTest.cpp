#include "components/MapLoader.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/ObstacleSpawner.h"
#include "components/events/SpawnEvents.h"
#include "enums/BonusType.h"
#include "enums/GameMode.h"
#include "entities/BaseObj.h"
#include "utils/UuidUtils.h"
#include "components/WorldGeometry.h"
#include <gtest/gtest.h>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>
#include "TestUtils.h"//NOTE: PrintTo for the Point types

// the loader turns a character grid into a map, or says what went wrong - each case feeds it one grid
class MapLoaderTest : public testing::Test
{
protected:
	//NOTE: three rows of four, one of each interesting kind - enough to tell a parse from a guess
	static constexpr auto kTinyMap{"# a comment\n"
			"\n"
			"0123\n"
			"4567\n"
			"0000\n"};

	//NOTE: the smallest grid the rules let through - three cells make a tank, so both edges need a gap that
	//wide, and the eagle takes a square of its own that wide too
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

	void TearDown() override
	{
		std::error_code ec;
		std::filesystem::remove(_tempMap, ec);
	}
};

TEST_F(MapLoaderTest, ReadsTheGridAndItsSize)
{
	const auto map{MapLoader::Parse(kTinyMap)};

	ASSERT_TRUE(map.has_value()) << map.error().reason;
	EXPECT_EQ(map->cols, 4u);
	EXPECT_EQ(map->rows, 3u);
	EXPECT_EQ(map->cells.size(), 12u);
}

TEST_F(MapLoaderTest, DigitsFollowTheLegendOrder)
{
	const auto map{MapLoader::Parse(kTinyMap)};

	ASSERT_TRUE(map.has_value()) << map.error().reason;
	EXPECT_EQ(map->At(0u, 0u), ObstacleType::None);
	EXPECT_EQ(map->At(1u, 0u), ObstacleType::Brick);
	EXPECT_EQ(map->At(2u, 0u), ObstacleType::Steel);
	EXPECT_EQ(map->At(3u, 0u), ObstacleType::Eagle);
	EXPECT_EQ(map->At(0u, 1u), ObstacleType::Fortress);
	EXPECT_EQ(map->At(1u, 1u), ObstacleType::Water);
	EXPECT_EQ(map->At(2u, 1u), ObstacleType::Bush);
	EXPECT_EQ(map->At(3u, 1u), ObstacleType::Ice);
}

TEST_F(MapLoaderTest, CrLfDoesNotBecomeAnExtraCell)
{
	const auto map{MapLoader::Parse("0123\r\n4567\r\n")};

	ASSERT_TRUE(map.has_value()) << map.error().reason;
	EXPECT_EQ(map->cols, 4u);
	EXPECT_EQ(map->rows, 2u);
}

TEST_F(MapLoaderTest, RaggedRowIsRejectedWithItsLineNumber)
{
	const auto map{MapLoader::Parse("# legend\n0000\n000\n")};

	ASSERT_FALSE(map.has_value());
	//NOTE: 1-based and counted over the whole file, comments included - that is what the editor shows
	EXPECT_EQ(map.error().line, 3u);
}

// a letter marks a bonus lying on the map: the cell under it stays free ground, the bonus is listed apart
TEST_F(MapLoaderTest, ALetterLaysABonusOnFreeGround)
{
	const auto map{MapLoader::Parse("0h00\n0000\n")};

	ASSERT_TRUE(map.has_value()) << map.error().reason;
	EXPECT_EQ(map->At(1u, 0u), ObstacleType::None);
	ASSERT_EQ(map->bonuses.size(), 1u);
	EXPECT_EQ(map->bonuses.front().col, 1u);
	EXPECT_EQ(map->bonuses.front().row, 0u);
	EXPECT_EQ(map->bonuses.front().type, BonusType::Helmet);
}

// the letters are a closed legend of their own - an unknown one is refused like an unknown digit
TEST_F(MapLoaderTest, ALetterOutsideTheBonusLegendIsRejected)
{
	const auto map{MapLoader::Parse("0000\n00z0\n")};

	ASSERT_FALSE(map.has_value());
	EXPECT_EQ(map.error().line, 2u);
}

TEST_F(MapLoaderTest, SymbolOutsideTheLegendIsRejected)
{
	const auto map{MapLoader::Parse("0000\n00x0\n")};

	ASSERT_FALSE(map.has_value());
	EXPECT_EQ(map.error().line, 2u);
}

TEST_F(MapLoaderTest, DigitPastTheLastObstacleIsRejected)
{
	//NOTE: '8' is one past Ice - a plain range check on the digit would have let it through as a cast
	const auto map{MapLoader::Parse("0080\n")};

	ASSERT_FALSE(map.has_value());
}

TEST_F(MapLoaderTest, CommentsOnlyIsNotAMap)
{
	const auto map{MapLoader::Parse("# just a legend\n#\n")};

	ASSERT_FALSE(map.has_value());
	EXPECT_EQ(map.error().line, 0u);
}

TEST_F(MapLoaderTest, MissingFileIsAnErrorNotAnEmptyMap)
{
	const auto map{MapLoader::LoadFromFile("Resources/Maps/there-is-no-such-level.map")};

	ASSERT_FALSE(map.has_value());
	EXPECT_EQ(map.error().path, "Resources/Maps/there-is-no-such-level.map");
}

//NOTE: nothing in the result says whether the disk was touched, so the file is swapped between the two
//loads - the first map coming back the second time is the cache answering
TEST_F(MapLoaderTest, TheSameFileIsReadFromDiskOnlyOnce)
{
	const std::string name{"battlecity_cache_" + UuidUtils::GetStringUuid(UuidUtils::GetRandomUuid()) + ".map"};
	const std::filesystem::path path{std::filesystem::temp_directory_path() / name};

	std::ofstream{path} << kPlayableGrid;
	const auto first{MapLoader::LoadFromFile(path)};

	//NOTE: both grids have to pass the rules - a refused second one errors whether the disk was read or not
	std::ofstream{path} << std::string{kPlayableGrid} + "000000000000\n";
	const auto second{MapLoader::LoadFromFile(path)};

	std::filesystem::remove(path);//NOTE: before the assertions - a failed one would return past it

	ASSERT_TRUE(first.has_value()) << first.error().reason;
	ASSERT_TRUE(second.has_value()) << second.error().reason;
	EXPECT_EQ(second->cols, 12u);
	EXPECT_EQ(second->rows, 8u);
}

// the rules of the game on top of the grid: a match needs its eagle and a way in at both ends
TEST_F(MapLoaderTest, AGridWithAnEagleAndBothEdgesOpenIsPlayable)
{
	const auto playable{ValidateGrid(std::string{kPlayableGrid})};

	EXPECT_TRUE(playable.has_value()) << playable.error().reason;
}

// no eagle is no match: nothing to defend, and nothing the enemy can win by
TEST_F(MapLoaderTest, AMapWithoutAnEagleIsRefused)
{
	std::string grid{kPlayableGrid};
	grid[grid.find('3')] = '0';

	EXPECT_FALSE(ValidateGrid(grid).has_value());
}

// two eagles leave the fortress and the win condition split between them
TEST_F(MapLoaderTest, ASecondEagleIsRefused)
{
	std::string grid{kPlayableGrid};
	grid[grid.find('0')] = '3';

	EXPECT_FALSE(ValidateGrid(grid).has_value());
}

// bush is ground a tank drives over, so a row of it along the edge is not a wall
TEST_F(MapLoaderTest, ABushEdgeStillLeavesRoomToSpawn)
{
	std::string grid{kPlayableGrid};
	grid.replace(0u, kRowStride - 1u, std::string(kRowStride - 1u, '6'));

	EXPECT_TRUE(ValidateGrid(grid).has_value());
}

// a walled-off edge leaves the spawner waiting for a spot that never comes
TEST_F(MapLoaderTest, ASealedTopEdgeLeavesNowhereToSpawn)
{
	std::string grid{kPlayableGrid};
	grid.replace(0u, kRowStride - 1u, std::string(kRowStride - 1u, '1'));

	EXPECT_FALSE(ValidateGrid(grid).has_value());
}

TEST_F(MapLoaderTest, ASealedBottomEdgeLeavesNowhereToSpawn)
{
	std::string grid{kPlayableGrid};
	grid.replace(kRowStride * 7u, kRowStride - 1u, std::string(kRowStride - 1u, '1'));

	EXPECT_FALSE(ValidateGrid(grid).has_value());
}

// the gap has to take a whole tank - two cells of the three is still a wall
TEST_F(MapLoaderTest, AGapNarrowerThanATankIsNoOpening)
{
	std::string row(kRowStride - 1u, '1');
	row.replace(4u, 2u, "00");

	std::string grid{kPlayableGrid};
	grid.replace(0u, row.size(), row);

	EXPECT_FALSE(ValidateGrid(grid).has_value());
}

//NOTE: the eagle is one character but a whole tank across, and the file leaves its body as zeros - read as
//free ground they would promise a spawn spot inside the fortress
TEST_F(MapLoaderTest, TheEaglesOwnBodyIsNotFreeGround)
{
	const auto grid{std::string{"00000\n"
			"00000\n"
			"00000\n"
			"03000\n"
			"00000\n"
			"00000\n"}};

	EXPECT_FALSE(ValidateGrid(grid).has_value()) << "the cells the eagle covers were read as free";
}

TEST(WorldGeometryTest, ClassicMapKeepsTheClassicField)
{
	const UPoint battlefieldSize{WorldGeometry::ForMap(52u, 50u)};

	EXPECT_EQ(battlefieldSize.x, 624u);
	EXPECT_EQ(battlefieldSize.y, 600u);
}

TEST(WorldGeometryTest, WideMapWidensTheWorldInsteadOfShrinkingTheCell)
{
	const UPoint battlefieldSize{WorldGeometry::ForMap(80u, 50u)};

	EXPECT_EQ(battlefieldSize.x, 960u);
	EXPECT_EQ(battlefieldSize.y, 600u);
}

TEST(WorldGeometryTest, TallMapMakesTheWorldTaller)
{
	const UPoint battlefieldSize{WorldGeometry::ForMap(52u, 100u)};

	EXPECT_EQ(battlefieldSize.x, 624u);
	EXPECT_EQ(battlefieldSize.y, 1200u);
}

TEST(WorldGeometryTest, EmptyMapProducesNoWorld)
{
	const UPoint battlefieldSize{WorldGeometry::ForMap(0u, 0u)};

	EXPECT_EQ(battlefieldSize.x, 0u);
	EXPECT_EQ(battlefieldSize.y, 0u);
}

TEST(WorldGeometryTest, LogicalSizeIsTheFieldPlusTheBar)
{
	GameConfig gameConfig{};
	gameConfig.battlefieldSize = WorldGeometry::ForMap(52u, 50u);

	EXPECT_EQ(gameConfig.LogicalSize().x, 624u + WorldGeometry::kSideBarWidth);
	EXPECT_EQ(gameConfig.LogicalSize().y, 600u);
}

//NOTE: the server sizes an eagle from the map, the client from a spawn event carrying only a position -
//they used to disagree, the client giving it a single cell
TEST(ObstacleSpawnerTest, ClientGivesTheEagleTheSameSpanTheMapDoes)
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
	EXPECT_DOUBLE_EQ(allObjects[0]->GetWidth(), cell * ObstacleCellSpan(ObstacleType::Eagle));
	EXPECT_DOUBLE_EQ(allObjects[1]->GetWidth(), cell);
}
