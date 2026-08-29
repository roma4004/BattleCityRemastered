#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/input/InputProviderForMenu.h"
#include "gtest/gtest.h"
#include <memory>

// InputProviderForMenu owns the pause flag; everything else only mirrors PauseStatusEvent. These
// tests pin that a pause set from outside (battlefield reset, window drag, host echo) reaches the
// owner, so it can still release it afterwards.
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

// Check that a game reset releases a pause that was set from outside the menu
TEST_F(PauseSyncTest, GameResetReleasesExternalPause)
{
	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	EXPECT_TRUE(_isPaused);

	_events->EmitEvent(GameResetEvent{});

	EXPECT_FALSE(_isPaused);
}

// Check that the pause toggle sees an external pause instead of flipping past it
TEST_F(PauseSyncTest, ToggleReleasesExternalPause)
{
	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	EXPECT_TRUE(_isPaused);

	_events->EmitEvent(PauseReleasedEvent{});

	EXPECT_FALSE(_isPaused);
}

// Check that the same request twice doesn't flip the flag back
TEST_F(PauseSyncTest, RepeatedRequestIsIdempotent)
{
	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	_events->EmitEvent(SetPauseEvent{.isPaused = true});

	EXPECT_TRUE(_isPaused);

	_events->EmitEvent(SetPauseEvent{.isPaused = false});

	EXPECT_FALSE(_isPaused);
}
