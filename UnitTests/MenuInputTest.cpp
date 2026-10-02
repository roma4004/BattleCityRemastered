#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "components/input/InputProviderForMenu.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

// the menu keys are read on PreTickUpdate, so every case here presses what it needs, runs one tick and
// counts what came out of it - the mode is applied at most once per press
class MenuInputTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	GameConfig _gameConfig{};
	std::unique_ptr<InputProviderForMenu> _menuInput{nullptr};

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

// one press of the menu key opens it, and applies nothing
TEST_F(MenuInputTest, MenuKeyOnlyTogglesTheMenu)
{
	_events->EmitEvent(MenuReleasedEvent{});
	_events->EmitEvent(PreTickUpdateEvent{});

	ASSERT_EQ(_menuShown.size(), 1u);
	EXPECT_TRUE(_menuShown.front());
	EXPECT_EQ(_applyCount, 0);
}

// a second press closes it again - still nothing applied, so leaving the menu is not a choice
TEST_F(MenuInputTest, MenuKeyClosesWhatItOpenedWithoutApplyingAnything)
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
TEST_F(MenuInputTest, ConfirmAppliesTheModeAndDoesNotHideTheMenuItself)
{
	_events->EmitEvent(MenuReleasedEvent{});
	_events->EmitEvent(EnterEvent{.isPressed = true});
	_events->EmitEvent(PreTickUpdateEvent{});

	EXPECT_EQ(_applyCount, 1);
	EXPECT_EQ(_showMenuCount, 0);
}

//NOTE: the mouse used to send only the press half, so the flag latched and re-applied every frame
TEST_F(MenuInputTest, ReleasingConfirmStopsItFromApplyingAgain)
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
TEST_F(MenuInputTest, ConfirmIsIgnoredWhileTheMenuIsClosed)
{
	_events->EmitEvent(EnterEvent{.isPressed = true});
	_events->EmitEvent(PreTickUpdateEvent{});

	EXPECT_EQ(_applyCount, 0);
}

//NOTE: a client's unpause goes to the server - the screen standing in for the menu must not send one
TEST_F(MenuInputTest, HidingTheMenuForTheServerScreenKeepsTheMatchPaused)
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
