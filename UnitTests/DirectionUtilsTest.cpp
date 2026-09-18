#include "utils/DirectionUtils.h"
#include "enums/Direction.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include <gtest/gtest.h>

// every case here works one 20x20 rectangle standing at 100,100 and asks what a step of 5 does to it -
// where it lands, what it sweeps, how far it is from a neighbour or from the edge of a 500x400 field
class DirectionUtilsTest : public testing::Test
{
protected:
	static constexpr ObjRectangle kRect{.x = 100.0, .y = 100.0, .w = 20.0, .h = 20.0};
	static constexpr UPoint kField{.x = 500, .y = 400};
	static constexpr double kStep{5.0};
};

// sweep a 20x20 rect by 5 in each direction: the box spans from where the step lands to the edge it
// started at, so a collision check sees everything the step passes through
TEST_F(DirectionUtilsTest, SweptCoversTheStartAndTheStep)
{
	const ObjRectangle up{DirectionUtils::Swept(kRect, kStep, Direction::UP)};
	EXPECT_DOUBLE_EQ(up.y, 95.0);
	EXPECT_DOUBLE_EQ(up.Bottom(), kRect.Bottom());

	const ObjRectangle left{DirectionUtils::Swept(kRect, kStep, Direction::LEFT)};
	EXPECT_DOUBLE_EQ(left.x, 95.0);
	EXPECT_DOUBLE_EQ(left.Right(), kRect.Right());

	const ObjRectangle down{DirectionUtils::Swept(kRect, kStep, Direction::DOWN)};
	EXPECT_DOUBLE_EQ(down.y, kRect.y);
	EXPECT_DOUBLE_EQ(down.Bottom(), 125.0);

	const ObjRectangle right{DirectionUtils::Swept(kRect, kStep, Direction::RIGHT)};
	EXPECT_DOUBLE_EQ(right.x, kRect.x);
	EXPECT_DOUBLE_EQ(right.Right(), 125.0);
}

// step the same rect by 5 four ways: only x or y moves, width and height stay
TEST_F(DirectionUtilsTest, MovedKeepsTheSize)
{
	//NOTE: constexpr only promises - a call in a constant expression is what proves it
	static_assert(DirectionUtils::Unit(Direction::LEFT).x == -1.0);
	static_assert(DirectionUtils::Moved(ObjRectangle{.y = 1.0, .w = 2.0, .h = 2.0}, 3.0, Direction::DOWN).y == 4.0);
	static_assert(ObjRectangle{.x = 1.0, .y = 1.0, .w = 2.0, .h = 2.0}.GetScaledBy(2.0).x == 0.0);

	for (const Direction dir: {Direction::UP, Direction::LEFT, Direction::DOWN, Direction::RIGHT})
	{
		const ObjRectangle moved{DirectionUtils::Moved(kRect, kStep, dir)};
		EXPECT_DOUBLE_EQ(moved.w, kRect.w);
		EXPECT_DOUBLE_EQ(moved.h, kRect.h);
	}

	EXPECT_DOUBLE_EQ(DirectionUtils::Moved(kRect, kStep, Direction::UP).y, 95.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::Moved(kRect, kStep, Direction::LEFT).x, 95.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::Moved(kRect, kStep, Direction::DOWN).y, 105.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::Moved(kRect, kStep, Direction::RIGHT).x, 105.0);
}

// step a point by 3 four ways and expect the offsets the rectangle overload gives
TEST_F(DirectionUtilsTest, MovedPointFollowsTheSameTable)
{
	constexpr FPoint center{.x = 10.0, .y = 10.0};

	EXPECT_DOUBLE_EQ(DirectionUtils::Moved(center, 3.0, Direction::UP).y, 7.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::Moved(center, 3.0, Direction::DOWN).y, 13.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::Moved(center, 3.0, Direction::LEFT).x, 7.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::Moved(center, 3.0, Direction::RIGHT).x, 13.0);
}

// put a neighbour above, below, left and right and ask for the gap towards it: measured from the
// edge that leads the movement, so a 20 gap up and a 40 gap down for the same spacing
TEST_F(DirectionUtilsTest, GapToMeasuresFromTheLeadingEdge)
{
	static_assert(DirectionUtils::GapTo(ObjRectangle{.x = 5.0, .w = 1.0, .h = 1.0}, ObjRectangle{.w = 2.0, .h = 1.0},
										Direction::LEFT) == 3.0);

	constexpr ObjRectangle above{.x = 100.0, .y = 60.0, .w = 20.0, .h = 20.0};
	EXPECT_DOUBLE_EQ(DirectionUtils::GapTo(kRect, above, Direction::UP), 20.0);

	constexpr ObjRectangle below{.x = 100.0, .y = 160.0, .w = 20.0, .h = 20.0};
	EXPECT_DOUBLE_EQ(DirectionUtils::GapTo(kRect, below, Direction::DOWN), 40.0);

	constexpr ObjRectangle onTheLeft{.x = 60.0, .y = 100.0, .w = 20.0, .h = 20.0};
	EXPECT_DOUBLE_EQ(DirectionUtils::GapTo(kRect, onTheLeft, Direction::LEFT), 20.0);

	constexpr ObjRectangle onTheRight{.x = 160.0, .y = 100.0, .w = 20.0, .h = 20.0};
	EXPECT_DOUBLE_EQ(DirectionUtils::GapTo(kRect, onTheRight, Direction::RIGHT), 40.0);
}

// overlap the two rects and expect a negative gap, not zero - zero would read as "touching"
TEST_F(DirectionUtilsTest, GapToIsNegativeWhenTheTargetIsBehindTheEdge)
{
	constexpr ObjRectangle overlapping{.x = 100.0, .y = 90.0, .w = 20.0, .h = 20.0};

	EXPECT_LT(DirectionUtils::GapTo(kRect, overlapping, Direction::UP), 0.0);
}

// the same rect in a 500x400 field: the distance to each border
TEST_F(DirectionUtilsTest, GapToEdgeMeasuresTheBattlefieldBorder)
{
	EXPECT_DOUBLE_EQ(DirectionUtils::GapToEdge(kRect, kField, Direction::UP), 100.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::GapToEdge(kRect, kField, Direction::LEFT), 100.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::GapToEdge(kRect, kField, Direction::DOWN), 280.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::GapToEdge(kRect, kField, Direction::RIGHT), 380.0);
}

// step exactly onto the border in all four directions, then one step shorter and one longer
TEST_F(DirectionUtilsTest, FitsBeforeEdgeLetsAStepLandOnTheNearBorderOnly)
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

// a 40x10 rect: vertical asks give the height, horizontal ones the width
TEST_F(DirectionUtilsTest, SizeAlongPicksTheAxisOfMovement)
{
	constexpr ObjRectangle wide{.x = 0.0, .y = 0.0, .w = 40.0, .h = 10.0};

	EXPECT_DOUBLE_EQ(DirectionUtils::SizeAlong(wide, Direction::UP), 10.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::SizeAlong(wide, Direction::DOWN), 10.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::SizeAlong(wide, Direction::LEFT), 40.0);
	EXPECT_DOUBLE_EQ(DirectionUtils::SizeAlong(wide, Direction::RIGHT), 40.0);
}
