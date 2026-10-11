#include "components/managers/AnimationManager.h"
#include "components/AnimatedObjects.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "geometry/ObjRectangle.h"
#include "enums/AnimationType.h"
#include "enums/DrawLayer.h"
#include <algorithm>
#include <array>
#include <memory>
#include <ranges>
#include <span>

namespace
{
bool UpdateFrame(AnimatedObject& object)
{
	if (object.markToDispose || object.speed <= 0)
	{
		return false;
	}

	if (++object.ticksSinceLastFrame % object.speed != 0)
	{
		return false;
	}

	object.ticksSinceLastFrame = 0;
	if (++object.currentFrameIndex >= object.size)
	{
		object.currentFrameIndex = 0;

		if (object.passes != kEndlessAnimation && ++object.passesDone >= object.passes)
		{
			object.markToDispose = true;

			return true;
		}
	}

	return false;
}

AnimatedObject* FindReusable(std::vector<AnimatedObject>& container, const AnimationType type)
{
	const auto it{std::ranges::find_if(container, [type](const AnimatedObject& object)
	{
		return object.type == type && object.markToDispose;
	})};

	return it != container.end() ? std::to_address(it) : nullptr;
}
}//namespace

AnimationManager::AnimationManager(const std::shared_ptr<EventSystem>& events)
	: _events(events)
{
	_effectAnimations.reserve(100);
	_groundAnimations.reserve(10);

	Subscribe();
}

void AnimationManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnGameReset));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnPostTickUpdate));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnPauseStatus));

	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateTankSpawn));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateBonusSpawn));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCancelTankSpawn));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnMoveTankSpawn));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnTankSpawnCompleted));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnBonusSpawnCompleted));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateTankExplosion));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateBulletExplosion));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateWaterFlow));
}

void AnimationManager::OnGameReset(const GameResetEvent&) { Reset(); }

void AnimationManager::OnPauseStatus(const PauseStatusEvent& event) { _isPaused = event.isPaused; }

//NOTE: frames are counted in ticks, not in time, so the paused game clock does not stop them - the flag does
void AnimationManager::OnPostTickUpdate(const PostTickUpdateEvent&)
{
	if (_isPaused)
	{
		return;
	}

	//NOTE: water never ends, so it has nothing to report
	std::ranges::for_each(_groundAnimations, [](AnimatedObject& object) { UpdateFrame(object); });

	//NOTE: only a spawn burst has an owner, the uuid waiting for it; UpdateFrame reports the end once
	std::vector<Uuid> finished{};
	for (AnimatedObject& object: _effectAnimations)
	{
		if (const bool isFinished{UpdateFrame(object)};
			isFinished && object.owner != Uuid{})
		{
			finished.push_back(object.owner);
		}
	}

	//NOTE: after the sweep - a listener may start another burst, and the container must not grow mid-loop
	for (const Uuid owner: finished)
	{
		_events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = owner});
	}
}

std::span<const AnimatedObject> AnimationManager::OnLayer(const DrawLayer layer) const
{
	switch (layer)
	{
		case DrawLayer::Ground:
			return _groundAnimations;
		case DrawLayer::Effects:
			return _effectAnimations;
		case DrawLayer::World:
		case DrawLayer::Overlay:
			break;
	}

	return {};
}

void AnimationManager::OnCreateTankSpawn(const AnimationCreateTankSpawnEvent& event)
{
	CreateAnimation(AnimationType::Tank_Spawn, event.rect, {.owner = event.uuid, .isEndless = event.isEndless});
}

void AnimationManager::OnCreateBonusSpawn(const AnimationCreateBonusSpawnEvent& event)
{
	CreateAnimation(AnimationType::Bonus_Spawn, event.rect, {.owner = event.uuid, .isEndless = event.isEndless});
}

//NOTE: disposed is enough - UpdateFrame skips it, so it never reaches the frame that reports
void AnimationManager::OnCancelTankSpawn(const AnimationCancelTankSpawnEvent& event)
{
	Cancel(AnimationType::Tank_Spawn, event.uuid);
}

void AnimationManager::OnMoveTankSpawn(const AnimationMoveTankSpawnEvent& event)
{
	auto matching{_effectAnimations | std::views::filter([&event](const AnimatedObject& object)
	{
		return object.type == AnimationType::Tank_Spawn && object.owner == event.uuid && !object.markToDispose;
	})};

	std::ranges::for_each(matching, [&event](AnimatedObject& object) { object.rect = event.rect; });
}

//NOTE: a client's burst is endless and ends only here; the host's has already ended by the time it lands
void AnimationManager::OnTankSpawnCompleted(const TankSpawnCompletedEvent& event)
{
	Cancel(AnimationType::Tank_Spawn, event.uuid);
}

void AnimationManager::OnBonusSpawnCompleted(const BonusSpawnCompletedEvent& event)
{
	Cancel(AnimationType::Bonus_Spawn, event.uuid);
}

void AnimationManager::OnCreateTankExplosion(const AnimationCreateTankExplosionEvent& event)
{
	CreateAnimation(AnimationType::Tank_Explosion, event.rect);
}

void AnimationManager::OnCreateBulletExplosion(const AnimationCreateBulletExplosionEvent& event)
{
	CreateAnimation(AnimationType::Bullet_Explosion, event.rect);
}

void AnimationManager::OnCreateWaterFlow(const AnimationCreateWaterEvent& event)
{
	CreateAnimation(AnimationType::Water_Flow, event.rect);
}

void AnimationManager::Reset()
{
	_effectAnimations.clear();
	_groundAnimations.clear();
}

void AnimationManager::Place(const AnimatedObject& animation)
{
	auto& target{ContainerOf(animation.type)};
	if (auto* reusable{FindReusable(target, animation.type)})
	{
		*reusable = animation;

		return;
	}

	target.push_back(animation);
}

std::vector<AnimatedObject>& AnimationManager::ContainerOf(const AnimationType type)
{
	return type == AnimationType::Water_Flow ? _groundAnimations : _effectAnimations;
}

constexpr AnimationManager::AnimationPreset AnimationManager::GetPreset(const AnimationType type)
{
	static constexpr std::array s_presets{
			KeyValue{.type = AnimationType::Tank_Spawn,
					 .preset = {.size = 3, .scale = 16, .speed = 20}},
			KeyValue{.type = AnimationType::Tank_Explosion,
					 .preset = {.size = 2, .scale = 32, .speed = 30}},
			KeyValue{.type = AnimationType::Bullet_Explosion,
					 .preset = {.size = 3, .scale = 16, .speed = 20}},
			KeyValue{.type = AnimationType::Water_Flow,
					 .preset = {.size = 16, .scale = 1, .speed = 20, .passes = kEndlessAnimation}},
			KeyValue{.type = AnimationType::Bonus_Spawn,
					 .preset = {.size = 3, .scale = 16, .speed = 20, .passes = 2}},
	};

	static_assert(s_presets.size() == static_cast<std::size_t>(AnimationType::Count),
				  "Amount of presets should be equal to AnimationType.size()");

	static_assert(
			std::ranges::all_of(
					std::views::iota(std::size_t{0}, s_presets.size()),
					[](const std::size_t i) { return static_cast<std::size_t>(s_presets[i].type) == i; }),
			"Order of presets should be equal to enum AnimationType");

	return s_presets[static_cast<std::size_t>(type)].preset;
}

void AnimationManager::CreateAnimation(const AnimationType type, const ObjRectangle rect, const AnimationExtras& extras)
{
	const auto& [size, scale, speed, passes] = GetPreset(type);
	Place(AnimatedObject{.rect = rect,
						 .size = size,
						 .speed = speed,
						 .passes = extras.isEndless ? kEndlessAnimation : passes,
						 .owner = extras.owner,
						 .type = type,
						 .scale = scale});
}

void AnimationManager::Cancel(const AnimationType type, const Uuid owner)
{
	auto matching{ContainerOf(type) | std::views::filter([type, owner](const AnimatedObject& object)
	{
		return object.type == type && object.owner == owner;
	})};

	std::ranges::for_each(matching, [](AnimatedObject& object) { object.markToDispose = true; });
}
