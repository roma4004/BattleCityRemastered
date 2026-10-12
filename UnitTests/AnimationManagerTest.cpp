#include "components/AnimatedObjects.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "components/managers/AnimationManager.h"
#include "enums/AnimationType.h"
#include "enums/DrawLayer.h"
#include "geometry/ObjRectangle.h"
#include "utils/Uuid.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

// the spawn burst doubles as the countdown, so its last frame is what lands the tank: frames are advanced by hand
class AnimationManagerTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<AnimationManager> _animations{nullptr};
	std::vector<EventSubscription> _subs{};

	std::optional<Uuid> _finished{};

	//NOTE: a burst reports only when it has an owner, and a nil uuid is how the manager spells "none"
	Uuid _burstUuid{UuidUtils::GetRandomUuid()};
	ObjRectangle _rect{.x = 0.0, .y = 0.0, .w = 12.0, .h = 12.0};
	//NOTE: a spawn burst is three frames of twenty ticks, so this is well past its last one
	int _ticksPerBurst{120};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_animations = std::make_unique<AnimationManager>(_events);

		_subs.push_back(_events->AddListener([this](const SpawnAnimationFinishedEvent& event)
		{
			_finished = event.uuid;
		}));
	}

	void RunTicks(const int count) const
	{
		for (int tick = 0; tick < count; ++tick)
		{
			_events->EmitEvent(PostTickUpdateEvent{.deltaTime = 1.0 / 60.0});
		}
	}

	//NOTE: a disposed burst keeps its slot, but nobody paints it
	[[nodiscard]] bool IsPlaying(const AnimationType type) const
	{
		return std::ranges::any_of(_animations->OnLayer(DrawLayer::Effects), [type](const AnimatedObject& animation)
		{
			return animation.type == type && !animation.markToDispose;
		});
	}
};

// a burst that plays to the end names the object it belonged to
TEST_F(AnimationManagerTest, ABurstLeftAloneReportsItsOwner)
{
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = _rect, .uuid = _burstUuid});

	RunTicks(_ticksPerBurst);

	ASSERT_TRUE(_finished.has_value());
	EXPECT_EQ(*_finished, _burstUuid);
}

// a cancelled one says nothing at all
TEST_F(AnimationManagerTest, ACancelledBurstNeverReports)
{
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = _rect, .uuid = _burstUuid});

	_events->EmitEvent(AnimationCancelTankSpawnEvent{.uuid = _burstUuid});

	RunTicks(_ticksPerBurst);

	EXPECT_FALSE(_finished.has_value());
}

// and the cancel takes only that burst, not everything the owner has running
TEST_F(AnimationManagerTest, ACancelLeavesTheOwnersOtherAnimations)
{
	_events->EmitEvent(AnimationCreateBonusSpawnEvent{.rect = _rect, .uuid = _burstUuid});

	_events->EmitEvent(AnimationCancelTankSpawnEvent{.uuid = _burstUuid});

	RunTicks(_ticksPerBurst * 2);

	EXPECT_TRUE(_finished.has_value());
}

//NOTE: a client's burst waits for the server's word, so it has to outlast a burst of its own
TEST_F(AnimationManagerTest, AnEndlessTankBurstPlaysUntilTheTankLands)
{
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = _rect, .uuid = _burstUuid, .isEndless = true});

	RunTicks(_ticksPerBurst);

	EXPECT_FALSE(_finished.has_value());
	EXPECT_TRUE(IsPlaying(AnimationType::Tank_Spawn));

	_events->EmitEvent(TankSpawnCompletedEvent{.uuid = _burstUuid});

	EXPECT_FALSE(IsPlaying(AnimationType::Tank_Spawn));
}

// the same for a bonus
TEST_F(AnimationManagerTest, AnEndlessBonusBurstPlaysUntilTheBonusLands)
{
	_events->EmitEvent(AnimationCreateBonusSpawnEvent{.rect = _rect, .uuid = _burstUuid, .isEndless = true});

	RunTicks(_ticksPerBurst * 2);

	EXPECT_FALSE(_finished.has_value());
	EXPECT_TRUE(IsPlaying(AnimationType::Bonus_Spawn));

	_events->EmitEvent(BonusSpawnCompletedEvent{.uuid = _burstUuid});

	EXPECT_FALSE(IsPlaying(AnimationType::Bonus_Spawn));
}

//NOTE: a finished burst's slot is reused - a client that watched the demo first must not inherit its end
TEST_F(AnimationManagerTest, AnEndlessBurstInAReusedSlotStaysEndless)
{
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = _rect, .uuid = _burstUuid});
	RunTicks(_ticksPerBurst);
	ASSERT_TRUE(_finished.has_value());

	_finished.reset();
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = _rect, .uuid = _burstUuid, .isEndless = true});
	RunTicks(_ticksPerBurst);

	EXPECT_FALSE(_finished.has_value());
}
