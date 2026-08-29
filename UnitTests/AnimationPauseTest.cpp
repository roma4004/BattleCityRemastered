#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "components/managers/AnimationManager.h"
#include "geometry/ObjRectangle.h"
#include "gtest/gtest.h"
#include <memory>
#include <optional>

//NOTE: water is the animation that never ends, so it is still on screen to be asked about its frame
class AnimationPauseTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<AnimationManager> _animations{nullptr};
	EventSubscription _drawSub{};
	std::optional<int> _frame{};
	int _ticksPerFrame{20};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_animations = std::make_unique<AnimationManager>(_events);
		_drawSub = _events->AddListener([this](const DrawAnimationEvent& event) { _frame = event.frame; });

		_events->EmitEvent(AnimationCreateWaterEvent{.rect = ObjRectangle{.x = 0.f, .y = 0.f, .w = 12.f, .h = 12.f}});
	}
};

TEST_F(AnimationPauseTest, FrameAdvancesWhileRunning)
{
	_events->EmitEvent(DrawEvent{});
	ASSERT_TRUE(_frame.has_value());
	const int startFrame = *_frame;

	for (int tick = 0; tick < _ticksPerFrame; ++tick)
	{
		_events->EmitEvent(PostTickUpdateEvent{.deltaTime = 1.0 / 60.0});
	}

	_events->EmitEvent(DrawEvent{});

	EXPECT_NE(*_frame, startFrame);
}

TEST_F(AnimationPauseTest, FrameStandsStillWhilePaused)
{
	_events->EmitEvent(DrawEvent{});
	ASSERT_TRUE(_frame.has_value());
	const int startFrame = *_frame;

	_events->EmitEvent(PauseStatusEvent{.isPaused = true});

	for (int tick = 0; tick < _ticksPerFrame * 3; ++tick)
	{
		_events->EmitEvent(PostTickUpdateEvent{.deltaTime = 1.0 / 60.0});
	}

	_events->EmitEvent(DrawEvent{});

	EXPECT_EQ(*_frame, startFrame);
}

TEST_F(AnimationPauseTest, FrameResumesAfterUnpause)
{
	_events->EmitEvent(DrawEvent{});
	ASSERT_TRUE(_frame.has_value());
	const int startFrame = *_frame;

	_events->EmitEvent(PauseStatusEvent{.isPaused = true});
	_events->EmitEvent(PostTickUpdateEvent{.deltaTime = 1.0 / 60.0});
	_events->EmitEvent(PauseStatusEvent{.isPaused = false});

	for (int tick = 0; tick < _ticksPerFrame; ++tick)
	{
		_events->EmitEvent(PostTickUpdateEvent{.deltaTime = 1.0 / 60.0});
	}

	_events->EmitEvent(DrawEvent{});

	EXPECT_NE(*_frame, startFrame);
}
