#include "components/input/GamepadDirection.h"
#include "enums/Direction.h"
#include "gtest/gtest.h"
#include <optional>

// one pad drives one tank, and it has two ways of saying where: the d-pad and the left stick. Every case
// here works the same pad and asks what the tank should be following - a direction, or nothing at all
class GamepadDirectionTest : public testing::Test
{
protected:
	//NOTE: the stick rests near the centre and never exactly on it, so anything inside the dead zone is not
	//a press; the tilt is well past it
	static constexpr int kDeadZone{8000};
	static constexpr int kTilt{20000};

	GamepadDirection _direction{kDeadZone};
};

TEST_F(GamepadDirectionTest, AStickInsideTheDeadZoneHoldsNothing)
{
	_direction.MoveStickX(-kDeadZone);
	_direction.MoveStickY(kDeadZone);

	EXPECT_EQ(_direction.Held(), std::nullopt);
}

// tilt left and half-down: the bigger axis names the side; tilt further up and the other axis takes over
TEST_F(GamepadDirectionTest, TheDominantAxisNamesTheSide)
{
	_direction.MoveStickX(-kTilt);
	_direction.MoveStickY(kTilt / 2);
	EXPECT_EQ(_direction.Held(), Direction::LEFT);

	_direction.MoveStickY(-kTilt * 3 / 2);
	EXPECT_EQ(_direction.Held(), Direction::UP);
}

// hold up on the d-pad, tilt the stick right, then press left: whatever moved last drives the tank
TEST_F(GamepadDirectionTest, TheDirectionPressedLastWins)
{
	_direction.PressDpad(Direction::UP);
	_direction.MoveStickX(kTilt);
	EXPECT_EQ(_direction.Held(), Direction::RIGHT);

	_direction.PressDpad(Direction::LEFT);
	EXPECT_EQ(_direction.Held(), Direction::LEFT);
}

// let the newest go while the other is still held: the tank falls back to it instead of stopping
TEST_F(GamepadDirectionTest, LettingTheNewestGoHandsBackToWhatIsStillHeld)
{
	_direction.MoveStickX(kTilt);
	_direction.PressDpad(Direction::UP);
	_direction.ReleaseDpad(Direction::UP);
	EXPECT_EQ(_direction.Held(), Direction::RIGHT);

	_direction.MoveStickX(0);
	EXPECT_EQ(_direction.Held(), std::nullopt);
}

// a stick already tilted keeps sending values as the thumb shifts - those are not new presses, so the
// d-pad pressed afterwards keeps the tank
TEST_F(GamepadDirectionTest, AStickWobblingOnItsSideDoesNotTakeOverTheDpad)
{
	_direction.MoveStickX(kTilt);
	_direction.PressDpad(Direction::UP);
	_direction.MoveStickX(kTilt + 1000);

	EXPECT_EQ(_direction.Held(), Direction::UP);
}
