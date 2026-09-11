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

TEST_F(SeatAssignmentTest, TheNthDeviceDrivesTheNthSeat)
{
	EXPECT_EQ(SlotForDevice(0u, false), PlayerSlot::P1);
	EXPECT_EQ(SlotForDevice(1u, false), PlayerSlot::P2);
}

//NOTE: one flag for every device - a pair playing on pads has to swap with the keyboard, not against it
TEST_F(SeatAssignmentTest, TheSwapFlipsEveryDevice)
{
	EXPECT_EQ(SlotForDevice(0u, true), PlayerSlot::P2);
	EXPECT_EQ(SlotForDevice(1u, true), PlayerSlot::P1);
}

TEST_F(SeatAssignmentTest, TheDedicatedServerOwnsNoSeat)
{
	_gameConfig.gameMode = GameMode::PlayAsHost;

	ExpectSeatsOwned(false, false);
}

TEST_F(SeatAssignmentTest, AHotSeatOwnsBothHalves)
{
	_gameConfig.gameMode = GameMode::TwoPlayers;

	ExpectSeatsOwned(true, true);
}

//NOTE: the other tank mirrors the host's - a keyboard half wired to it would twitch under our keys
TEST_F(SeatAssignmentTest, AClientOwnsOnlyTheSeatItWasGiven)
{
	_gameConfig.gameMode = GameMode::PlayAsClient;
	_gameConfig.ownSlot = PlayerSlot::P2;

	ExpectSeatsOwned(false, true);
}

//NOTE: a default would hand the first seat to whoever asks before the host has answered
TEST_F(SeatAssignmentTest, AClientOwnsNothingBeforeTheSeatArrives)
{
	_gameConfig.gameMode = GameMode::PlayAsClient;

	ExpectSeatsOwned(false, false);
}
