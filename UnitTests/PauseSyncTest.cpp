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
	bool _isPaused{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_menuInput = std::make_unique<InputProviderForMenu>(_events, _gameConfig);
		_statusSub = _events->AddListener([this](const PauseStatusEvent& event) { _isPaused = event.isPaused; });
	}
};

// pause from outside, then reset the game: the pause is lifted, so a fresh match never starts frozen
TEST_F(PauseSyncTest, GameResetReleasesExternalPause)
{
	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	EXPECT_TRUE(_isPaused);

	_events->EmitEvent(GameResetEvent{});

	EXPECT_FALSE(_isPaused);
}

// pause from outside, then press the pause key: the key releases it instead of toggling into a
// second pause nobody can leave
TEST_F(PauseSyncTest, ToggleReleasesExternalPause)
{
	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	EXPECT_TRUE(_isPaused);

	_events->EmitEvent(PauseReleasedEvent{});

	EXPECT_FALSE(_isPaused);
}

// two identical pause requests, then one release - the extra request does not need a second release
TEST_F(PauseSyncTest, RepeatedRequestIsIdempotent)
{
	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	_events->EmitEvent(SetPauseEvent{.isPaused = true});

	EXPECT_TRUE(_isPaused);

	_events->EmitEvent(SetPauseEvent{.isPaused = false});

	EXPECT_FALSE(_isPaused);
}
