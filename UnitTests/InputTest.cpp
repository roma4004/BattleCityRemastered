#include "application/GameConfig.h"
#include "components/AddressField.h"
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
#include "network/Endpoints.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

using namespace std::string_view_literals;

// what the player drives with: which device drives which seat, the gamepad, the menu keys and the address field
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

// digits typed in land as the field shows them: octets and groups hand the caret on, past 255 or 65535 nothing more
TEST_F(InputTest, TypedDigitsLandWhereTheFieldShowsThem)
{
	struct Typed final
	{
		AddressFamily family;
		std::string_view typed;
		std::string_view shown;
	};

	constexpr std::array kTyped{
			Typed{.family = AddressFamily::IPv4, .typed = "1921681001", .shown = "192.168.100.1:auto"},
			Typed{.family = AddressFamily::IPv4, .typed = "300", .shown = "30.0.___.___:auto"},
			Typed{.family = AddressFamily::IPv4, .typed = ":65536", .shown = "___.___.___.___:6553"},
			Typed{.family = AddressFamily::IPv6, .typed = "20010db8", .shown = " 2001:0db8: :auto"},
			Typed{.family = AddressFamily::IPv6, .typed = "2001:", .shown = " 2001: :auto"}};
	for (const Typed& typed: kTyped)
	{
		SCOPED_TRACE(typed.typed);
		AddressField field{typed.family};

		std::ranges::for_each(typed.typed, [&field](const char symbol) { field.Type(symbol); });

		EXPECT_EQ(field.Shown(false).text, typed.shown);
	}
}

// a colon in IPv4, or digits past a whole IPv6, leave the address for the port
TEST_F(InputTest, TypingPastTheAddressGoesToThePort)
{
	struct Typed final
	{
		AddressFamily family;
		std::string_view typed;
		std::string_view text;
	};

	constexpr std::array kTyped{
			Typed{.family = AddressFamily::IPv4, .typed = "1:5", .text = "1...:5"},
			Typed{.family = AddressFamily::IPv6,
				  .typed = "1111222233334444555566667777888812",
				  .text = "[1111:2222:3333:4444:5555:6666:7777:8888]:12"}};
	for (const Typed& typed: kTyped)
	{
		SCOPED_TRACE(typed.typed);
		AddressField field{typed.family};

		std::ranges::for_each(typed.typed, [&field](const char symbol) { field.Type(symbol); });

		EXPECT_EQ(field.Text(), typed.text);
	}
}

// back on the row, the same colon is a new one - the added one was left behind with the row
TEST_F(InputTest, AColonAfterComingBackIsANewOne)
{
	AddressField field{AddressFamily::IPv6};

	std::ranges::for_each("2001"sv, [&field](const char symbol) { field.Type(symbol); });
	field.Focus();
	field.Type(':');

	EXPECT_EQ(field.Shown(false).text, " 2001:: :auto");
}

// a digit typed into a full group pushes the group's last digit over the colon into the next one
TEST_F(InputTest, ADigitInAFullGroupFlowsOverTheColon)
{
	AddressField field{AddressFamily::IPv6};
	std::ranges::for_each("20010db8"sv, [&field](const char symbol) { field.Type(symbol); });
	for (int step{}; step < 6; ++step)
	{
		field.Press(TextKey::CaretLeft);
	}

	field.Type('f');

	EXPECT_EQ(field.Shown(false).text, " 2001:f0db:8 :auto");
}

// erasing the colon between two full groups would join eight digits - the digit before it goes instead
TEST_F(InputTest, TheColonBetweenFullGroupsIsNotErased)
{
	AddressField field{AddressFamily::IPv6};
	std::ranges::for_each("20010db8"sv, [&field](const char symbol) { field.Type(symbol); });
	for (int step{}; step < 5; ++step)
	{
		field.Press(TextKey::CaretLeft);
	}

	field.Press(TextKey::Erase);

	EXPECT_EQ(field.Shown(false).text, " 200:0db8: :auto");
}

// a filled address goes in whole, with the caret on its port
TEST_F(InputTest, AFilledAddressPutsTheCaretOnThePort)
{
	AddressField field{AddressFamily::IPv4};

	field.Fill(network::ServerAddress{.host = "10.0.0.6", .port = 4001}, false);

	const ShownAddress shown{field.Shown(true)};
	EXPECT_EQ(shown.text, "10.0.0.6:4001");
	EXPECT_EQ(shown.caret, shown.text.size());
}

// a server's own address is picked, not typed - the caret starts on the port and typing goes there
TEST_F(InputTest, APickedAddressStartsTheCaretOnThePort)
{
	AddressField field{AddressFamily::IPv4, AddressInput::Picked};
	field.Fill(network::ServerAddress{.host = "192.168.0.5"}, false);

	field.CaretToStart();
	field.Type('9');

	EXPECT_EQ(field.Text(), "192.168.0.5:9");
}

// erasing past the port's start and stepping left of it leave the picked address alone
TEST_F(InputTest, APickedAddressKeepsTheCaretInThePort)
{
	AddressField field{AddressFamily::IPv4, AddressInput::Picked};
	field.Fill(network::ServerAddress{.host = "192.168.0.5", .port = 50}, false);

	constexpr std::array kKeys{TextKey::Erase, TextKey::Erase, TextKey::Erase, TextKey::CaretLeft, TextKey::WordLeft};
	for (const TextKey key: kKeys)
	{
		field.Press(key);
	}

	field.Type('7');

	EXPECT_EQ(field.Text(), "192.168.0.5:7");
}
