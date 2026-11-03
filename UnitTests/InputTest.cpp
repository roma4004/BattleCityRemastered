#include "application/GameConfig.h"
#include "components/input/GamepadDirection.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/PlayerSlot.h"
#include "gtest/gtest.h"
#include <array>
#include <cstddef>
#include <optional>
#include <string_view>

// what the player drives with: which device drives which seat and the gamepad
class InputTest : public testing::Test
{
protected:
	//NOTE: a resting stick is near the center, never on it - inside the dead zone is no press
	static constexpr int kDeadZone{8000};
	static constexpr int kTilt{20000};

	GameConfig _gameConfig{};
	GamepadDirection _direction{kDeadZone};
};

// devices take the seats in order; Tab swaps the keyboard halves and the first two pads, Shift+Tab the other two
TEST_F(InputTest, TheNthDeviceDrivesTheNthSeatUnlessSwapped)
{
	struct Swap final
	{
		bool isFirstPairSwapped{};
		bool isSecondPairSwapped{};
		std::array<PlayerSlot, 4> seats{};
	};

	constexpr std::array kSwaps{
			Swap{.seats = {PlayerSlot::P1, PlayerSlot::P2, PlayerSlot::P3, PlayerSlot::P4}},
			Swap{.isFirstPairSwapped = true,
				 .seats = {PlayerSlot::P2, PlayerSlot::P1, PlayerSlot::P3, PlayerSlot::P4}},
			Swap{.isSecondPairSwapped = true,
				 .seats = {PlayerSlot::P1, PlayerSlot::P2, PlayerSlot::P4, PlayerSlot::P3}}};
	for (const Swap& swap: kSwaps)
	{
		SCOPED_TRACE(swap.isFirstPairSwapped ? "Tab" : swap.isSecondPairSwapped ? "Shift+Tab" : "no swap");
		for (std::size_t device{}; device < swap.seats.size(); ++device)
		{
			EXPECT_EQ(SlotForDevice(device, swap.isFirstPairSwapped, swap.isSecondPairSwapped), swap.seats[device])
					<< "device " << device;
		}
	}
}

// the keys drive only the seats this machine owns - a client's other tank mirrors the server's and would twitch
TEST_F(InputTest, AMachineDrivesOnlyTheSeatsItOwns)
{
	struct Owner final
	{
		std::string_view why;
		GameMode mode;
		std::optional<PlayerSlot> ownSlot;
		bool isP1Owned{};
		bool isP2Owned{};
	};

	//NOTE: a client with no seat yet owns none - a default would hand the first seat to whoever asks first
	constexpr std::array kOwners{
			Owner{.why = "the dedicated server plays nothing", .mode = GameMode::PlayAsHost, .ownSlot = {}},
			Owner{.why = "a hot seat",
				  .mode = GameMode::TwoPlayers,
				  .ownSlot = {},
				  .isP1Owned = true,
				  .isP2Owned = true},
			Owner{.why = "a client given the second seat", .mode = GameMode::PlayAsClient, .ownSlot = PlayerSlot::P2,
				  .isP2Owned = true},
			Owner{.why = "a client before its seat arrives", .mode = GameMode::PlayAsClient, .ownSlot = {}}};
	for (const Owner& owner: kOwners)
	{
		SCOPED_TRACE(owner.why);
		_gameConfig.gameMode = owner.mode;
		_gameConfig.ownSlot = owner.ownSlot;

		EXPECT_EQ(_gameConfig.IsOwnSlot(PlayerSlot::P1), owner.isP1Owned);
		EXPECT_EQ(_gameConfig.IsOwnSlot(PlayerSlot::P2), owner.isP2Owned);
	}
}

// a stick resting inside the dead zone holds no direction at all
TEST_F(InputTest, AStickInsideTheDeadZoneHoldsNothing)
{
	_direction.MoveStickX(-kDeadZone);
	_direction.MoveStickY(kDeadZone);

	EXPECT_EQ(_direction.Held(), std::nullopt);
}

// tilt left and half-down: the bigger axis names the side; tilt further up and the other axis takes over
TEST_F(InputTest, TheDominantAxisNamesTheSide)
{
	_direction.MoveStickX(-kTilt);
	_direction.MoveStickY(kTilt / 2);
	EXPECT_EQ(_direction.Held(), Direction::LEFT);

	_direction.MoveStickY(-kTilt * 3 / 2);
	EXPECT_EQ(_direction.Held(), Direction::UP);
}

// hold up on the d-pad, tilt the stick right, then press left: whatever moved last drives the tank
TEST_F(InputTest, TheDirectionPressedLastWins)
{
	_direction.PressDpad(Direction::UP);
	_direction.MoveStickX(kTilt);
	EXPECT_EQ(_direction.Held(), Direction::RIGHT);

	_direction.PressDpad(Direction::LEFT);
	EXPECT_EQ(_direction.Held(), Direction::LEFT);
}

// let the newest go while the other is still held: the tank falls back to it instead of stopping
TEST_F(InputTest, LettingTheNewestGoHandsBackToWhatIsStillHeld)
{
	_direction.MoveStickX(kTilt);
	_direction.PressDpad(Direction::UP);
	_direction.ReleaseDpad(Direction::UP);
	EXPECT_EQ(_direction.Held(), Direction::RIGHT);

	_direction.MoveStickX(0);
	EXPECT_EQ(_direction.Held(), std::nullopt);
}

// a tilted stick keeps sending values as the thumb shifts - no new presses, so a later d-pad press keeps the tank
TEST_F(InputTest, AStickWobblingOnItsSideDoesNotTakeOverTheDpad)
{
	_direction.MoveStickX(kTilt);
	_direction.PressDpad(Direction::UP);
	_direction.MoveStickX(kTilt + 1000);

	EXPECT_EQ(_direction.Held(), Direction::UP);
}
