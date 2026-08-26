#include "application/GameConfig.h"
#include "application/ProjectConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/events/TimingEvents.h"
#include "components/input/InputProviderForMenu.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

class MenuInputTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	ProjectConfig _projectConfig{"", true};
	GameConfig _gameConfig{_projectConfig};
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
		_subs.push_back(_events->AddListener([this](const MenuShowedEvent& event)
		{
			_menuShown.push_back(event.isShown);
		}));
	}
};

TEST_F(MenuInputTest, MenuKeyOnlyTogglesTheMenu)
{
	_events->EmitEvent(MenuReleasedEvent{});
	_events->EmitEvent(PreTickUpdateEvent{});

	ASSERT_EQ(_menuShown.size(), 1u);
	EXPECT_TRUE(_menuShown.front());
	EXPECT_EQ(_applyCount, 0);
}

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

//NOTE: hiding follows the match starting, not the apply - a host with no client stays in the lobby
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

TEST_F(MenuInputTest, ConfirmIsIgnoredWhileTheMenuIsClosed)
{
	_events->EmitEvent(EnterEvent{.isPressed = true});
	_events->EmitEvent(PreTickUpdateEvent{});

	EXPECT_EQ(_applyCount, 0);
}
