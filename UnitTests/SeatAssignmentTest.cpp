#include "application/GameConfig.h"
#include "enums/GameMode.h"
#include "enums/PlayerSlot.h"
#include <gtest/gtest.h>

class SeatAssignmentTest : public testing::Test
{
protected:
	GameConfig _gameConfig{};

	void ExpectSeatsOwned(const bool p1, const bool p2) const
	{
		EXPECT_EQ(_gameConfig.IsOwnSlot(PlayerSlot::P1), p1) << "player one";
		EXPECT_EQ(_gameConfig.IsOwnSlot(PlayerSlot::P2), p2) << "player two";
	}
};

// without the swap the first device takes player one, the second player two
TEST_F(SeatAssignmentTest, TheNthDeviceDrivesTheNthSeat)
{
	EXPECT_EQ(SlotForDevice(0u, false, false), PlayerSlot::P1);
	EXPECT_EQ(SlotForDevice(1u, false, false), PlayerSlot::P2);
	EXPECT_EQ(SlotForDevice(2u, false, false), PlayerSlot::P3);
	EXPECT_EQ(SlotForDevice(3u, false, false), PlayerSlot::P4);
}

//NOTE: the swap covers the keyboard halves and the first two pads
TEST_F(SeatAssignmentTest, TheSwapFlipsTheFirstTwoDevicesOnly)
{
	EXPECT_EQ(SlotForDevice(0u, true, false), PlayerSlot::P2);
	EXPECT_EQ(SlotForDevice(1u, true, false), PlayerSlot::P1);
	EXPECT_EQ(SlotForDevice(2u, true, false), PlayerSlot::P3);
	EXPECT_EQ(SlotForDevice(3u, true, false), PlayerSlot::P4);
}

// and Shift+Tab flips the other two, leaving the first pair where it was
TEST_F(SeatAssignmentTest, TheSecondSwapFlipsTheLastTwoDevicesOnly)
{
	EXPECT_EQ(SlotForDevice(0u, false, true), PlayerSlot::P1);
	EXPECT_EQ(SlotForDevice(1u, false, true), PlayerSlot::P2);
	EXPECT_EQ(SlotForDevice(2u, false, true), PlayerSlot::P4);
	EXPECT_EQ(SlotForDevice(3u, false, true), PlayerSlot::P3);
}

// the spawned server plays nothing, so neither half is its own
TEST_F(SeatAssignmentTest, TheDedicatedServerOwnsNoSeat)
{
	_gameConfig.gameMode = GameMode::PlayAsHost;

	ExpectSeatsOwned(false, false);
}

// two players at one keyboard own both
TEST_F(SeatAssignmentTest, AHotSeatOwnsBothHalves)
{
	_gameConfig.gameMode = GameMode::TwoPlayers;

	ExpectSeatsOwned(true, true);
}

//NOTE: the other tank mirrors the server's - a keyboard half wired to it would twitch under our keys
TEST_F(SeatAssignmentTest, AClientOwnsOnlyTheSeatItWasGiven)
{
	_gameConfig.gameMode = GameMode::PlayAsClient;
	_gameConfig.ownSlot = PlayerSlot::P2;

	ExpectSeatsOwned(false, true);
}

//NOTE: a default would hand the first seat to whoever asks before the server has answered
TEST_F(SeatAssignmentTest, AClientOwnsNothingBeforeTheSeatArrives)
{
	_gameConfig.gameMode = GameMode::PlayAsClient;

	ExpectSeatsOwned(false, false);
}
