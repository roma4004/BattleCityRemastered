#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/input/InputProviderForMenu.h"
#include "gtest/gtest.h"
#include <memory>

// InputProviderForMenu owns the pause flag and everything else mirrors PauseStatusEvent, so a pause set
// from outside - a reset, a window drag, a host echo - has to reach the owner for it to be released
class PauseSyncTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<InputProviderForMenu> _menuInput{nullptr};
	GameConfig _gameConfig{};
	EventSubscription _statusSub{};
	bool _isPaused{false};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_menuInput = std::make_unique<InputProviderForMenu>(_events, _gameConfig);
		_statusSub = _events->AddListener([this](const PauseStatusEvent& event) { _isPaused = event.isPaused; });
	}
};

// A game reset releases a pause that was set from outside the menu
TEST_F(PauseSyncTest, GameResetReleasesExternalPause)
{
	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	EXPECT_TRUE(_isPaused);

	_events->EmitEvent(GameResetEvent{});

	EXPECT_FALSE(_isPaused);
}

// The pause toggle sees an external pause instead of flipping past it
TEST_F(PauseSyncTest, ToggleReleasesExternalPause)
{
	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	EXPECT_TRUE(_isPaused);

	_events->EmitEvent(PauseReleasedEvent{});

	EXPECT_FALSE(_isPaused);
}

// The same request twice does not flip the flag back
TEST_F(PauseSyncTest, RepeatedRequestIsIdempotent)
{
	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	_events->EmitEvent(SetPauseEvent{.isPaused = true});

	EXPECT_TRUE(_isPaused);

	_events->EmitEvent(SetPauseEvent{.isPaused = false});

	EXPECT_FALSE(_isPaused);
}
