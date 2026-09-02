#include "utils/DirectionUtils.h"
#include "enums/Direction.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include <gtest/gtest.h>

namespace
{
constexpr ObjRectangle kRect{.x = 100.0, .y = 100.0, .w = 20.0, .h = 20.0};
constexpr UPoint kField{.x = 500, .y = 400};
constexpr double kStep = 5.0;
}// namespace

TEST(DirectionUtilsTest, SweepCoversTheStartAndTheStep)
{
	const ObjRectangle up = DirectionUtils::Sweep(kRect, kStep, Direction::UP);
	EXPECT_DOUBLE_EQ(up.y, 95.0);
	EXPECT_DOUBLE_EQ(up.Bottom(), kRect.Bottom());

	const ObjRectangle left = DirectionUtils::Sweep(kRect, kStep, Direction::LEFT);
	EXPECT_DOUBLE_EQ(left.x, 95.0);
	EXPECT_DOUBLE_EQ(left.Right(), kRect.Right());

	const ObjRectangle down = DirectionUtils::Sweep(kRect, kStep, Direction::DOWN);
	EXPECT_DOUBLE_EQ(down.y, kRect.y);
	EXPECT_DOUBLE_EQ(down.Bottom(), 125.0);

	const ObjRectangle right = DirectionUtils::Sweep(kRect, kStep, Direction::RIGHT);
	EXPECT_DOUBLE_EQ(right.x, kRect.x);
	EXPECT_DOUBLE_EQ(right.Right(), 125.0);
}

TEST(DirectionUtilsTest, AdvanceKeepsTheSize)
{
	for (const Direction dir: {Direction::UP, Direction::LEFT, Direction::DOWN, Direction::RIGHT})
	{
		const ObjRectangle moved = DirectionUtils::Advance(kRect, kStep, dir);
		EXPECT_DOUBLE_EQ(moved.w, kRect.w);
		EXPECT_DOUBLE_EQ(moved.h, kRect.h);
	}

	EXPECT_DOUBLE_EQ(DirectionUtils::Advance(kRect, kStep, Direction::UP).y, 95.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::Advance(kRect, kStep, Direction::LEFT).x, 95.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::Advance(kRect, kStep, Direction::DOWN).y, 105.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::Advance(kRect, kStep, Direction::RIGHT).x, 105.0);
}

TEST(DirectionUtilsTest, AdvancedPointFollowsTheSameTable)
{
	constexpr FPoint center{.x = 10.0, .y = 10.0};

	EXPECT_DOUBLE_EQ(DirectionUtils::Advance(center, 3.0, Direction::UP).y, 7.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::Advance(center, 3.0, Direction::DOWN).y, 13.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::Advance(center, 3.0, Direction::LEFT).x, 7.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::Advance(center, 3.0, Direction::RIGHT).x, 13.0);
}

TEST(DirectionUtilsTest, GapToMeasuresFromTheLeadingEdge)
{
	constexpr ObjRectangle above{.x = 100.0, .y = 60.0, .w = 20.0, .h = 20.0};
	EXPECT_DOUBLE_EQ(DirectionUtils::GapTo(kRect, above, Direction::UP), 20.0);

	constexpr ObjRectangle below{.x = 100.0, .y = 160.0, .w = 20.0, .h = 20.0};
	EXPECT_DOUBLE_EQ(DirectionUtils::GapTo(kRect, below, Direction::DOWN), 40.0);

	constexpr ObjRectangle onTheLeft{.x = 60.0, .y = 100.0, .w = 20.0, .h = 20.0};
	EXPECT_DOUBLE_EQ(DirectionUtils::GapTo(kRect, onTheLeft, Direction::LEFT), 20.0);

	constexpr ObjRectangle onTheRight{.x = 160.0, .y = 100.0, .w = 20.0, .h = 20.0};
	EXPECT_DOUBLE_EQ(DirectionUtils::GapTo(kRect, onTheRight, Direction::RIGHT), 40.0);
}

TEST(DirectionUtilsTest, GapToIsNegativeWhenTheTargetIsBehindTheEdge)
{
	constexpr ObjRectangle overlapping{.x = 100.0, .y = 90.0, .w = 20.0, .h = 20.0};

	EXPECT_LT(DirectionUtils::GapTo(kRect, overlapping, Direction::UP), 0.0);
}

TEST(DirectionUtilsTest, GapToEdgeMeasuresTheBattlefieldBorder)
{
	EXPECT_DOUBLE_EQ(DirectionUtils::GapToEdge(kRect, kField, Direction::UP), 100.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::GapToEdge(kRect, kField, Direction::LEFT), 100.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::GapToEdge(kRect, kField, Direction::DOWN), 280.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::GapToEdge(kRect, kField, Direction::RIGHT), 380.0);
}

TEST(DirectionUtilsTest, FitsBeforeEdgeLetsAStepLandOnTheNearBorderOnly)
{
	constexpr ObjRectangle rect{.x = 10.0, .y = 10.0, .w = 20.0, .h = 20.0};
	constexpr UPoint field{.x = 100, .y = 100};

	// a step exactly onto the border: allowed towards the origin, refused away from it
	EXPECT_TRUE(DirectionUtils::FitsBeforeEdge(rect, field, 10.0, Direction::UP));
	EXPECT_TRUE(DirectionUtils::FitsBeforeEdge(rect, field, 10.0, Direction::LEFT));
	EXPECT_FALSE(DirectionUtils::FitsBeforeEdge(rect, field, 70.0, Direction::DOWN));
	EXPECT_FALSE(DirectionUtils::FitsBeforeEdge(rect, field, 70.0, Direction::RIGHT));

	EXPECT_TRUE(DirectionUtils::FitsBeforeEdge(rect, field, 69.0, Direction::DOWN));
	EXPECT_FALSE(DirectionUtils::FitsBeforeEdge(rect, field, 11.0, Direction::UP));
}

TEST(DirectionUtilsTest, SideAlongPicksTheAxisOfMovement)
{
	constexpr ObjRectangle wide{.x = 0.0, .y = 0.0, .w = 40.0, .h = 10.0};

	EXPECT_DOUBLE_EQ(DirectionUtils::SideAlong(wide, Direction::UP), 10.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::SideAlong(wide, Direction::DOWN), 10.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::SideAlong(wide, Direction::LEFT), 40.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::SideAlong(wide, Direction::RIGHT), 40.0);
}
