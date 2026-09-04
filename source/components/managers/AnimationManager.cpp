#include "components/managers/AnimationManager.h"
#include "utils/Log.h"
#include "components/AnimatedObjects.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
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

	for (AnimatedObject& object: _autoAnimatedObjects)
	{
		//NOTE: an owner is what a spawn animation has and an explosion does not - it is the uuid waiting
		//for this burst to end, and UpdateFrame says true once, on the tick it does
		if (const bool isFinished = UpdateFrame(object);
			isFinished && object.owner != Uuid{})
		{
			_events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = object.owner});
		}
	}
}

namespace
{
constexpr auto IsEnabled = [](const AnimatedObject& object) { return !object.markToDispose; };
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
	CreateAnimation(AnimationType::Tank_Spawn, event.rect, Author::None, event.uuid);
}

void AnimationManager::OnCreateBonusSpawn(const AnimationCreateBonusSpawnEvent& event)
{
	CreateAnimation(AnimationType::Bonus_Spawn, event.rect, Author::None, event.uuid);
}

//NOTE: disposed is enough - UpdateFrame skips it, so it never reaches the frame that reports
void AnimationManager::OnCancelTankSpawn(const AnimationCancelTankSpawnEvent& event)
{
	auto matching = _autoAnimatedObjects | std::views::filter([uuid = event.uuid](const AnimatedObject& object)
	{
		return object.owner == uuid;
	});

	std::ranges::for_each(matching, [](AnimatedObject& object) { object.markToDispose = true; });
}

void AnimationManager::OnCreateTankMove(const AnimationCreateTankMoveEvent& event)
{
	CreateAnimation(AnimationType::Tank_Move, event.rect, event.author);
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
	const Author author = event.author;
	const auto it = std::ranges::find_if(_turnBasedTankObjects, [author](const AnimatedObject& object)
	{
		return object.author == author;
	});

	if (it == _turnBasedTankObjects.end())
	{
		return;
	}

	//Update tank animation position and dir
	it->rect.x = event.pos.x;
	it->rect.y = event.pos.y;
	it->dir = event.dir;

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
	_turnBasedTankObjects.clear();//NOTE: all tank_animation will be removed when tank died
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
							   .author = object.author});
}

void AnimationManager::Create(const Author author, const ObjRectangle rect, const AnimationType type,
							  const int size, const int scale, const int speed, const int passes, const Uuid owner)
{
	//NOTE: chose animation container for water if not then tanks, if not then other objects
	auto& target =
			type == AnimationType::Water_Flow
				? _autoAnimatedWaterObjects
				: type == AnimationType::Tank_Move
				? _turnBasedTankObjects
				: _autoAnimatedObjects;

	if (auto* reusable = FindReusable(target, type))
	{
		reusable->rect = rect;
		reusable->dir = {};
		reusable->currentFrameIndex = 0;
		reusable->ticksSinceLastFrame = 0;
		reusable->passesDone = 0;
		reusable->owner = owner;
		reusable->author = author;
		reusable->markToDispose = false;
	}
	else
	{
		target.emplace_back(author, rect, type, size, scale, speed, passes, owner);
	}
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
									   const Uuid owner)
{
	if (type == AnimationType::Tank_Explosion)
	{
		DisableTankAnimation(author);//NOTE: for tank we need to disable previous animation
	}

	const auto& [size, scale, speed, passes] = GetPreset(type);
	Create(author, rect, type, size, scale, speed, passes, owner);
}

bool AnimationManager::UpdateFrame(AnimatedObject& object)
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

void AnimationManager::OnHelmetEffect(const Author author, const bool isEnable)
{
	if (!isEnable)
	{
		DisableHelmetEffect(author);

		return;
	}

	const auto tankIt = std::ranges::find_if(_turnBasedTankObjects, [author](const AnimatedObject& tankObject)
	{
		return tankObject.author == author;
	});

	if (tankIt == _turnBasedTankObjects.end())
	{
		Log::Error("AnimationManager: no turn-based tank " + std::string{ToString(author)}
				   + " for the helmet effect");

		return;
	}

	//enable and update if exist
	const auto helmetIt = std::ranges::find_if(_autoAnimatedObjects, [author](const AnimatedObject& animatedObject)
	{
		return animatedObject.type == AnimationType::Helmet_Effect && animatedObject.author == author;
	});

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
	const auto it = std::ranges::find_if(_autoAnimatedObjects, [author](const AnimatedObject& object)
	{
		return !object.markToDispose
			   && object.type == AnimationType::Helmet_Effect
			   && object.author == author;
	});

	if (it != _autoAnimatedObjects.end())
	{
		//Update helmet animation position
		it->rect.x = pos.x;
		it->rect.y = pos.y;
	}
}

void AnimationManager::DisableTankAnimation(const Author author)
{
	auto matching = _turnBasedTankObjects | std::views::filter([author](const AnimatedObject& object)
	{
		return object.author == author;
	});

	std::ranges::for_each(matching, [](AnimatedObject& object) { object.markToDispose = true; });
}

void AnimationManager::DisableHelmetEffect(const Author author)
{
	auto matching = _autoAnimatedObjects | std::views::filter([author](const AnimatedObject& object)
	{
		return !object.markToDispose
			   && object.type == AnimationType::Helmet_Effect
			   && object.author == author;
	});

	std::ranges::for_each(matching, [](AnimatedObject& object) { object.markToDispose = true; });
}

AnimatedObject* AnimationManager::FindReusable(std::vector<AnimatedObject>& container, const AnimationType type)
{
	const auto it = std::ranges::find_if(container, [type](const AnimatedObject& object)
	{
		return object.type == type && object.markToDispose;
	});

	return it != container.end() ? std::to_address(it) : nullptr;
}
