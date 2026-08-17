#include "components/MapLoader.h"
#include "components/WorldGeometry.h"
#include <gtest/gtest.h>
#include <filesystem>

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

TEST(WorldGeometryTest, ClassicMapKeepsTheClassicCellAndSideBar)
{
	//NOTE: 52x50 in an 800x600 window is what the game shipped with - the refactor must not move it
	const WorldGeometry geometry = WorldGeometry::FitMap(UPoint{.x = 800u, .y = 600u}, 52u, 50u);

	EXPECT_FLOAT_EQ(geometry.cellSize, 12.f);
	EXPECT_EQ(geometry.battlefieldSize.x, 624u);
	EXPECT_EQ(geometry.battlefieldSize.y, 600u);
	EXPECT_EQ(geometry.sideBarWidth, 176u);
}

TEST(WorldGeometryTest, WideMapSqueezesTheSideBarFirst)
{
	const WorldGeometry geometry = WorldGeometry::FitMap(UPoint{.x = 800u, .y = 600u}, 80u, 50u);

	//NOTE: the bar gave up everything it could, and only then did the cell shrink below 12
	EXPECT_EQ(geometry.sideBarWidth, WorldGeometry::kMinSideBarWidth);
	EXPECT_FLOAT_EQ(geometry.cellSize, 8.5f);
}

TEST(WorldGeometryTest, TallMapLeavesTheBottomEmptyRatherThanStretching)
{
	const WorldGeometry geometry = WorldGeometry::FitMap(UPoint{.x = 800u, .y = 600u}, 52u, 100u);

	//NOTE: the cell stays square, so half the height of the window is simply not used
	EXPECT_FLOAT_EQ(geometry.cellSize, 6.f);
	EXPECT_EQ(geometry.battlefieldSize.y, 600u);
	EXPECT_EQ(geometry.battlefieldSize.x, 312u);
}

TEST(WorldGeometryTest, EmptyMapProducesNothingToFit)
{
	const WorldGeometry geometry = WorldGeometry::FitMap(UPoint{.x = 800u, .y = 600u}, 0u, 0u);

	EXPECT_FLOAT_EQ(geometry.cellSize, 0.f);
}
