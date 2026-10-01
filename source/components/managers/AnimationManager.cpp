#include "components/managers/AnimationManager.h"
#include "utils/Log.h"
#include "components/AnimatedObjects.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/Author.h"
#include "geometry/ObjRectangle.h"
#include "enums/AnimationType.h"
#include "geometry/Point.h"
#include <algorithm>
#include <array>
#include <memory>
#include <ranges>
#include <string>

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

constexpr auto IsEnabled = [](const AnimatedObject& object) { return !object.markToDispose; };

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
	_autoAnimatedObjects.reserve(100);
	_turnBasedTankObjects.reserve(6);
	_autoAnimatedWaterObjects.reserve(10);

	Subscribe();
}

void AnimationManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnGameReset));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnPostTickUpdate));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnPauseStatus));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnDraw));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnPostDraw));

	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateTankSpawn));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateBonusSpawn));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCancelTankSpawn));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnMoveTankSpawn));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnTankSpawnCompleted));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnBonusSpawnCompleted));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateTankExplosion));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateBulletExplosion));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateTankMove));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateWaterFlow));

	_subs.push_back(_events->AddListener(this, &AnimationManager::OnUpdateTankMove));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnHelmetEffect));
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
	std::ranges::for_each(_autoAnimatedWaterObjects, [](AnimatedObject& object) { UpdateFrame(object); });

	//NOTE: only a spawn burst has an owner, the uuid waiting for it; UpdateFrame reports the end once
	std::vector<Uuid> finished{};
	for (AnimatedObject& object: _autoAnimatedObjects)
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

void AnimationManager::OnDraw(const DrawEvent&) const
{
	const auto drawObject = [this](const AnimatedObject& object) { DrawObject(object); };

	std::ranges::for_each(_autoAnimatedWaterObjects | std::views::filter(IsEnabled), drawObject);
	std::ranges::for_each(_turnBasedTankObjects | std::views::filter(IsEnabled), drawObject);
}

//NOTE: later than the walls, which subscribe after this manager and would paint over every blast;
//still earlier than the bush, which draws later in this same phase - cover is meant to hide
void AnimationManager::OnPostDraw(const PostDrawEvent&) const
{
	std::ranges::for_each(_autoAnimatedObjects | std::views::filter(IsEnabled),
						  [this](const AnimatedObject& object) { DrawObject(object); });
}

void AnimationManager::OnCreateTankSpawn(const AnimationCreateTankSpawnEvent& event)
{
	CreateAnimation(AnimationType::Tank_Spawn, event.rect, Author::None,
					{.owner = event.uuid, .isEndless = event.isEndless});
}

void AnimationManager::OnCreateBonusSpawn(const AnimationCreateBonusSpawnEvent& event)
{
	CreateAnimation(AnimationType::Bonus_Spawn, event.rect, Author::None,
					{.owner = event.uuid, .isEndless = event.isEndless});
}

//NOTE: disposed is enough - UpdateFrame skips it, so it never reaches the frame that reports
void AnimationManager::OnCancelTankSpawn(const AnimationCancelTankSpawnEvent& event)
{
	Cancel(AnimationType::Tank_Spawn, event.uuid);
}

void AnimationManager::OnMoveTankSpawn(const AnimationMoveTankSpawnEvent& event)
{
	auto matching{_autoAnimatedObjects | std::views::filter([&event](const AnimatedObject& object)
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

void AnimationManager::OnCreateTankMove(const AnimationCreateTankMoveEvent& event)
{
	CreateAnimation(AnimationType::Tank_Move, event.rect, event.author, {.model = event.model, .tier = event.tier});
	OnHelmetEffect(event.author, true);
}

void AnimationManager::OnCreateTankExplosion(const AnimationCreateTankExplosionEvent& event)
{
	CreateAnimation(AnimationType::Tank_Explosion, event.rect, event.author);
}

void AnimationManager::OnCreateBulletExplosion(const AnimationCreateBulletExplosionEvent& event)
{
	CreateAnimation(AnimationType::Bullet_Explosion, event.rect, Author::None);
}

void AnimationManager::OnCreateWaterFlow(const AnimationCreateWaterEvent& event)
{
	CreateAnimation(AnimationType::Water_Flow, event.rect, Author::None);
}

void AnimationManager::OnUpdateTankMove(const AnimationTankUpdateEvent& event)
{
	const Author author{event.author};
	const auto it{std::ranges::find_if(_turnBasedTankObjects, [author](const AnimatedObject& object)
	{
		return object.author == author;
	})};

	if (it == _turnBasedTankObjects.end())
	{
		return;
	}

	it->rect.x = event.pos.x;
	it->rect.y = event.pos.y;
	it->dir = event.dir;
	it->tier = event.tier;

	UpdateFrame(*it);
	UpdateHelmetEffect(author, event.pos);
}

void AnimationManager::OnHelmetEffect(const AnimationBonusHelmetChangeEvent& event)
{
	OnHelmetEffect(event.author, event.isEnable);
}

void AnimationManager::Reset()
{
	_autoAnimatedObjects.clear();
	_turnBasedTankObjects.clear();
	_autoAnimatedWaterObjects.clear();
}

void AnimationManager::DrawObject(const AnimatedObject& object) const
{
	_events->EmitEvent(
			DrawAnimationEvent{.rect = object.rect,
							   .dir = object.dir,
							   .frame = object.currentFrameIndex,
							   .scale = object.scale,
							   .type = object.type,
							   .author = object.author,
							   .model = object.model,
							   .tier = object.tier});
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
	return type == AnimationType::Water_Flow
			   ? _autoAnimatedWaterObjects
			   : type == AnimationType::Tank_Move
			   ? _turnBasedTankObjects
			   : _autoAnimatedObjects;
}

constexpr AnimationManager::AnimationPreset AnimationManager::GetPreset(const AnimationType type)
{
	static constexpr std::array s_presets{
			KeyValue{.type = AnimationType::Tank_Spawn,
					 .preset = {.size = 3, .scale = 16, .speed = 20}},
			KeyValue{.type = AnimationType::Tank_Move,
					 .preset = {.size = 2, .scale = 16, .speed = 2, .passes = kEndlessAnimation}},
			KeyValue{.type = AnimationType::Tank_Explosion,
					 .preset = {.size = 2, .scale = 32, .speed = 30}},
			KeyValue{.type = AnimationType::Bullet_Explosion,
					 .preset = {.size = 3, .scale = 16, .speed = 20}},
			KeyValue{.type = AnimationType::Water_Flow,
					 .preset = {.size = 16, .scale = 1, .speed = 20, .passes = kEndlessAnimation}},
			KeyValue{.type = AnimationType::Helmet_Effect,
					 .preset = {.size = 2, .scale = 16, .speed = 20, .passes = kEndlessAnimation}},
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

void AnimationManager::CreateAnimation(const AnimationType type, const ObjRectangle rect, const Author author,
									   const AnimationExtras& extras)
{
	if (type == AnimationType::Tank_Explosion)
	{
		DisableTankAnimation(author);
	}

	const auto& [size, scale, speed, passes] = GetPreset(type);
	Place(AnimatedObject{.rect = rect,
						 .size = size,
						 .speed = speed,
						 .passes = extras.isEndless ? kEndlessAnimation : passes,
						 .owner = extras.owner,
						 .type = type,
						 .scale = scale,
						 .author = author,
						 .model = extras.model,
						 .tier = extras.tier});
}

void AnimationManager::OnHelmetEffect(const Author author, const bool isEnable)
{
	if (!isEnable)
	{
		DisableHelmetEffect(author);

		return;
	}

	const auto tankIt{std::ranges::find_if(_turnBasedTankObjects, [author](const AnimatedObject& tankObject)
	{
		return tankObject.author == author;
	})};

	if (tankIt == _turnBasedTankObjects.end())
	{
		Log::Error("AnimationManager: no turn-based tank " + std::string{ToString(author)}
				   + " for the helmet effect");

		return;
	}

	const auto helmetIt{std::ranges::find_if(_autoAnimatedObjects, [author](const AnimatedObject& animatedObject)
	{
		return animatedObject.type == AnimationType::Helmet_Effect && animatedObject.author == author;
	})};

	if (helmetIt == _autoAnimatedObjects.end())
	{
		CreateAnimation(AnimationType::Helmet_Effect, tankIt->rect, author);
		return;
	}

	helmetIt->markToDispose = false;
	helmetIt->rect.x = tankIt->rect.x;
	helmetIt->rect.y = tankIt->rect.y;
}

void AnimationManager::UpdateHelmetEffect(const Author author, const FPoint& pos)
{
	const auto it{std::ranges::find_if(_autoAnimatedObjects, [author](const AnimatedObject& object)
	{
		return !object.markToDispose
			   && object.type == AnimationType::Helmet_Effect
			   && object.author == author;
	})};

	if (it != _autoAnimatedObjects.end())
	{
		it->rect.x = pos.x;
		it->rect.y = pos.y;
	}
}

void AnimationManager::Cancel(const AnimationType type, const Uuid owner)
{
	auto matching{ContainerOf(type) | std::views::filter([type, owner](const AnimatedObject& object)
	{
		return object.type == type && object.owner == owner;
	})};

	std::ranges::for_each(matching, [](AnimatedObject& object) { object.markToDispose = true; });
}

void AnimationManager::DisableTankAnimation(const Author author)
{
	auto matching{_turnBasedTankObjects | std::views::filter([author](const AnimatedObject& object)
	{
		return object.author == author;
	})};

	std::ranges::for_each(matching, [](AnimatedObject& object) { object.markToDispose = true; });
}

void AnimationManager::DisableHelmetEffect(const Author author)
{
	auto matching{_autoAnimatedObjects | std::views::filter([author](const AnimatedObject& object)
	{
		return !object.markToDispose
			   && object.type == AnimationType::Helmet_Effect
			   && object.author == author;
	})};

	std::ranges::for_each(matching, [](AnimatedObject& object) { object.markToDispose = true; });
}
