#include "components/MapLoader.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/ObstacleSpawner.h"
#include "components/events/SpawnEvents.h"
#include "enums/GameMode.h"
#include "entities/BaseObj.h"
#include "utils/UuidUtils.h"
#include "components/WorldGeometry.h"
#include <gtest/gtest.h>
#include <filesystem>
#include "TestUtils.h"//NOTE: PrintTo for the Point types

namespace
{
//NOTE: three rows of four, one of each interesting kind - enough to tell a parse from a guess
constexpr auto kTinyMap = "# a comment\n"
						  "\n"
						  "0123\n"
						  "4567\n"
						  "0000\n";
}

TEST(MapLoaderTest, ReadsTheGridAndItsSize)
{
	const auto map = MapLoader::Parse(kTinyMap);

	ASSERT_TRUE(map.has_value()) << map.error().reason;
	EXPECT_EQ(map->cols, 4u);
	EXPECT_EQ(map->rows, 3u);
	EXPECT_EQ(map->cells.size(), 12u);
}

TEST(MapLoaderTest, DigitsFollowTheLegendOrder)
{
	const auto map = MapLoader::Parse(kTinyMap);

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

TEST(MapLoaderTest, CrLfDoesNotBecomeAnExtraCell)
{
	const auto map = MapLoader::Parse("0123\r\n4567\r\n");

	ASSERT_TRUE(map.has_value()) << map.error().reason;
	EXPECT_EQ(map->cols, 4u);
	EXPECT_EQ(map->rows, 2u);
}

TEST(MapLoaderTest, RaggedRowIsRejectedWithItsLineNumber)
{
	const auto map = MapLoader::Parse("# legend\n0000\n000\n");

	ASSERT_FALSE(map.has_value());
	//NOTE: 1-based and counted over the whole file, comments included - that is what the editor shows
	EXPECT_EQ(map.error().line, 3u);
}

TEST(MapLoaderTest, SymbolOutsideTheLegendIsRejected)
{
	const auto map = MapLoader::Parse("0000\n00x0\n");

	ASSERT_FALSE(map.has_value());
	EXPECT_EQ(map.error().line, 2u);
}

TEST(MapLoaderTest, DigitPastTheLastObstacleIsRejected)
{
	//NOTE: '8' is one past Ice - a plain range check on the digit would have let it through as a cast
	const auto map = MapLoader::Parse("0080\n");

	ASSERT_FALSE(map.has_value());
}

TEST(MapLoaderTest, CommentsOnlyIsNotAMap)
{
	const auto map = MapLoader::Parse("# just a legend\n#\n");

	ASSERT_FALSE(map.has_value());
	EXPECT_EQ(map.error().line, 0u);
}

TEST(MapLoaderTest, MissingFileIsAnErrorNotAnEmptyMap)
{
	const auto map = MapLoader::LoadFromFile("Resources/Maps/there-is-no-such-level.map");

	ASSERT_FALSE(map.has_value());
	EXPECT_EQ(map.error().path, "Resources/Maps/there-is-no-such-level.map");
}

TEST(MapLoaderTest, ShippedLevelOneParses)
{
	constexpr auto path = "Resources/Maps/level1.map";
	if (!std::filesystem::exists(path))
	{
		GTEST_SKIP() << "assets are copied next to the game exe, not next to the test one";
	}

	const auto map = MapLoader::LoadFromFile(path);

	ASSERT_TRUE(map.has_value()) << map.error().reason;
	EXPECT_EQ(map->cols, 52u);
	EXPECT_EQ(map->rows, 50u);
}

TEST(WorldGeometryTest, ClassicMapKeepsTheClassicField)
{
	const UPoint battlefieldSize = WorldGeometry::ForMap(52u, 50u);

	EXPECT_EQ(battlefieldSize.x, 624u);
	EXPECT_EQ(battlefieldSize.y, 600u);
}

TEST(WorldGeometryTest, WideMapWidensTheWorldInsteadOfShrinkingTheCell)
{
	const UPoint battlefieldSize = WorldGeometry::ForMap(80u, 50u);

	EXPECT_EQ(battlefieldSize.x, 960u);
	EXPECT_EQ(battlefieldSize.y, 600u);
}

TEST(WorldGeometryTest, TallMapMakesTheWorldTaller)
{
	const UPoint battlefieldSize = WorldGeometry::ForMap(52u, 100u);

	EXPECT_EQ(battlefieldSize.x, 624u);
	EXPECT_EQ(battlefieldSize.y, 1200u);
}

TEST(WorldGeometryTest, EmptyMapProducesNoWorld)
{
	const UPoint battlefieldSize = WorldGeometry::ForMap(0u, 0u);

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

//NOTE: the host sizes an eagle from the map, the client from a spawn event carrying only a position -
//they used to disagree, the client giving it a single cell
TEST(ObstacleSpawnerTest, ClientGivesTheEagleTheSameSpanTheMapDoes)
{
	auto events = std::make_shared<EventSystem>();
	std::vector<std::shared_ptr<BaseObj>> allObjects;
	auto spawnQueueSub = TestUtils::WireSpawnQueue(events, allObjects);

	GameConfig gameConfig{};
	gameConfig.gameMode = GameMode::PlayAsClient;
	const ObstacleSpawner spawner{events, gameConfig};

	const double cell{gameConfig.gridOffset};
	events->EmitEvent(ObstacleSpawnedEvent{.pos = {.x = 0.0, .y = 0.0}, .type = ObstacleType::Eagle,
										   .uuid = UuidUtils::GetRandomUuid()});
	events->EmitEvent(ObstacleSpawnedEvent{.pos = {.x = 0.0, .y = 0.0}, .type = ObstacleType::Brick,
										   .uuid = UuidUtils::GetRandomUuid()});

	ASSERT_EQ(allObjects.size(), 2u);
	EXPECT_DOUBLE_EQ(allObjects[0]->GetWidth(), cell * ObstacleCellSpan(ObstacleType::Eagle));
	EXPECT_DOUBLE_EQ(allObjects[1]->GetWidth(), cell);
}
