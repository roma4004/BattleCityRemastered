#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "components/input/GamepadDirection.h"
#include "components/input/InputProviderForMenu.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/PlayerSlot.h"
#include "gtest/gtest.h"
#include <array>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

// what the player drives with: which device drives which seat, the gamepad and the menu keys
class InputTest : public testing::Test
{
protected:
	//NOTE: a resting stick is near the center, never on it - inside the dead zone is no press
	static constexpr int kDeadZone{8000};
	static constexpr int kTilt{20000};

	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	GameConfig _gameConfig{};
	std::unique_ptr<InputProviderForMenu> _menuInput{nullptr};
	GamepadDirection _direction{kDeadZone};

	int _applyCount{};
	int _showMenuCount{};
	std::vector<bool> _menuShown{};
	std::vector<EventSubscription> _subs{};

	void SetUp() override
	{
		_gameConfig.gameMode = GameMode::OnePlayer;
		_menuInput = std::make_unique<InputProviderForMenu>(_events, _gameConfig);

		_subs.push_back(_events->AddListener([this](const ApplyGameModeEvent&) { ++_applyCount; }));
		_subs.push_back(_events->AddListener([this](const ShowMenuEvent&) { ++_showMenuCount; }));
		_subs.push_back(_events->AddListener([this](const MenuShownEvent& event)
		{
			_menuShown.push_back(event.isShown);
		}));
	}
};

// devices take the seats in order; Tab swaps the keyboard halves and the first two pads, Shift+Tab the other two
TEST_F(InputTest, TheNthDeviceDrivesTheNthSeatUnlessSwapped)
{
	struct Swap final
	{
		bool isFirstPairSwapped{};
		bool isSecondPairSwapped{};
		std::array<PlayerSlot, 4> seats;
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

// one press of the menu key opens it and a second closes it - nothing applied, so leaving the menu is not a choice
TEST_F(InputTest, MenuKeyClosesWhatItOpenedWithoutApplyingAnything)
{
	_events->EmitEvent(MenuReleasedEvent{});
	_events->EmitEvent(MenuReleasedEvent{});
	_events->EmitEvent(PreTickUpdateEvent{});

	ASSERT_EQ(_menuShown.size(), 2u);
	EXPECT_TRUE(_menuShown.front());
	EXPECT_FALSE(_menuShown.back());
	EXPECT_EQ(_applyCount, 0);
}

//NOTE: hiding follows the match starting, not the apply - a server with no client stays in the lobby
TEST_F(InputTest, ConfirmAppliesTheModeAndDoesNotHideTheMenuItself)
{
	_events->EmitEvent(MenuReleasedEvent{});
	_events->EmitEvent(EnterEvent{.isPressed = true});
	_events->EmitEvent(PreTickUpdateEvent{});

	EXPECT_EQ(_applyCount, 1);
	EXPECT_EQ(_showMenuCount, 0);
}

//NOTE: the mouse used to send only the press half, so the flag latched and re-applied every frame
TEST_F(InputTest, ReleasingConfirmStopsItFromApplyingAgain)
{
	_events->EmitEvent(MenuReleasedEvent{});
	_events->EmitEvent(EnterEvent{.isPressed = true});
	_events->EmitEvent(PreTickUpdateEvent{});
	_events->EmitEvent(EnterEvent{.isPressed = false});
	_events->EmitEvent(PreTickUpdateEvent{});
	_events->EmitEvent(PreTickUpdateEvent{});

	EXPECT_EQ(_applyCount, 1);
}

// confirm without opening the menu first has nothing to apply
TEST_F(InputTest, ConfirmIsIgnoredWhileTheMenuIsClosed)
{
	_events->EmitEvent(EnterEvent{.isPressed = true});
	_events->EmitEvent(PreTickUpdateEvent{});

	EXPECT_EQ(_applyCount, 0);
}

//NOTE: a client's unpause goes to the server - the screen standing in for the menu must not send one
TEST_F(InputTest, HidingTheMenuForTheServerScreenKeepsTheMatchPaused)
{
	_gameConfig.gameMode = GameMode::PlayAsClient;
	_gameConfig.gameState = GameState::Playing;
	_events->EmitEvent(MenuReleasedEvent{});
	std::vector<bool> requested{};
	const EventSubscription sub{_events->AddListener([&requested](const PauseRequestedEvent& event)
	{
		requested.push_back(event.isPaused);
	})};

	_events->EmitEvent(ServerScreenShownEvent{.isShown = true});
	_events->EmitEvent(ShowMenuEvent{.isShown = false});

	EXPECT_TRUE(_menuInput->GetPause());
	EXPECT_TRUE(requested.empty());
}
