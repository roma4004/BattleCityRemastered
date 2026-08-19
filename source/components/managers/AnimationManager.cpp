#include "components/managers/AnimationManager.h"
#include "utils/Log.h"
#include "components/AnimatedObjects.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "geometry/ObjRectangle.h"
#include "enums/AnimationType.h"
#include "geometry/Point.h"
#include <algorithm>
#include <array>
#include <memory>
#include <ranges>

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
	//TODO: draw explosion animation after others obstacle and tanks, maybe split explosions and other collections
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnDraw));

	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateTankSpawn));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateTankExplosion));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateBulletExplosion));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateTankMove));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnCreateWaterFlow));

	_subs.push_back(_events->AddListener(this, &AnimationManager::OnUpdateTankMove));
	_subs.push_back(_events->AddListener(this, &AnimationManager::OnHelmetEffect));
}

void AnimationManager::OnGameReset(const GameResetEvent&) { Reset(); }

void AnimationManager::OnPostTickUpdate(const PostTickUpdateEvent&)
{
	std::ranges::for_each(_autoAnimatedWaterObjects, UpdateFrame);
	std::ranges::for_each(_autoAnimatedObjects, UpdateFrame);
}

void AnimationManager::OnDraw(const DrawEvent&) const
{
	constexpr auto isEnabled = [](const AnimatedObject& object) { return !object.markToDispose; };
	const auto drawObject = [this](const AnimatedObject& object) { DrawObject(object); };

	std::ranges::for_each(_autoAnimatedWaterObjects | std::views::filter(isEnabled), drawObject);
	std::ranges::for_each(_turnBasedTankObjects | std::views::filter(isEnabled), drawObject);
	std::ranges::for_each(_autoAnimatedObjects | std::views::filter(isEnabled), drawObject);
}

void AnimationManager::OnCreateTankSpawn(const AnimationCreateTankSpawnEvent& event)
{
	CreateAnimation(AnimationType::Tank_Spawn, event.rect, event.name);
}

void AnimationManager::OnCreateTankMove(const AnimationCreateTankMoveEvent& event)
{
	CreateAnimation(AnimationType::Tank_Move, event.rect, event.name);
	OnHelmetEffect(event.name, true);
}

void AnimationManager::OnCreateTankExplosion(const AnimationCreateTankExplosionEvent& event)
{
	CreateAnimation(AnimationType::Tank_Explosion, event.rect, event.name);
}

void AnimationManager::OnCreateBulletExplosion(const AnimationCreateBulletExplosionEvent& event)
{
	CreateAnimation(AnimationType::Bullet_Explosion, event.rect, event.name);
}

void AnimationManager::OnCreateWaterFlow(const AnimationCreateWaterEvent& event)
{
	CreateAnimation(AnimationType::Water_Flow, event.rect, "Water");
}

void AnimationManager::OnUpdateTankMove(const AnimationTankUpdateEvent& event)
{
	const auto& name = event.name;
	const auto it = std::ranges::find_if(_turnBasedTankObjects, [&name](const AnimatedObject& object)
	{
		return object.name.ends_with(name);
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
	UpdateHelmetEffect(name, event.pos);
}

void AnimationManager::OnHelmetEffect(const AnimationBonusHelmetChangeEvent& event)
{
	OnHelmetEffect(event.name, event.isEnable);
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
							   .name = object.name});
}

void AnimationManager::Create(const std::string& name, const ObjRectangle rect, const AnimationType type,
							  const int size, const int scale, const int speed, const bool isInfinite)
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
		reusable->name = name;
		reusable->markToDispose = false;
	}
	else
	{
		target.emplace_back(name, rect, type, size, scale, speed, isInfinite);
	}
}

constexpr AnimationManager::AnimationPreset AnimationManager::GetPreset(const AnimationType type)
{
	static constexpr std::array s_presets{
			KeyValue{.type = AnimationType::Tank_Spawn,
					 .preset = {.name = "TankSpawn", .size = 3, .scale = 16, .speed = 20, .isInfinite = false}},
			KeyValue{.type = AnimationType::Tank_Move,
					 .preset = {.name = "", .size = 2, .scale = 16, .speed = 2, .isInfinite = true}},
			KeyValue{.type = AnimationType::Tank_Explosion,
					 .preset = {.name = "TankExplosion", .size = 2, .scale = 32, .speed = 30, .isInfinite = false}},
			KeyValue{.type = AnimationType::Bullet_Explosion,
					 .preset = {.name = "BulletExplosion", .size = 3, .scale = 16, .speed = 20, .isInfinite = false}},
			KeyValue{.type = AnimationType::Water_Flow,
					 .preset = {.name = "Water", .size = 16, .scale = 1, .speed = 20, .isInfinite = true}},
			KeyValue{.type = AnimationType::Helmet_Effect,
					 .preset = {.name = "", .size = 2, .scale = 16, .speed = 20, .isInfinite = true}},
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

void AnimationManager::CreateAnimation(const AnimationType type, const ObjRectangle rect, const std::string& name)
{
	if (type == AnimationType::Tank_Explosion)
	{
		DisableTankAnimation(name);//NOTE: for tank we need to disable previous animation
	}

	const auto& [presetName, size, scale, speed, isInfinite] = GetPreset(type);
	std::string animName{};
	if (type == AnimationType::Helmet_Effect)
	{
		animName = name + "HelmetEffect";
	}
	else if (type == AnimationType::Tank_Move)
	{
		animName = name;
	}
	else
	{
		animName = presetName;
	}

	Create(animName, rect, type, size, scale, speed, isInfinite);
}

void AnimationManager::UpdateFrame(AnimatedObject& object)
{
	if (object.markToDispose || object.speed <= 0)
	{
		return;
	}

	if (++object.ticksSinceLastFrame % object.speed != 0)
	{
		return;
	}

	object.ticksSinceLastFrame = 0;
	if (++object.currentFrameIndex >= object.size)
	{
		if (object.isInfinite == false)
		{
			object.markToDispose = true;
		}

		object.currentFrameIndex = 0;
	}
}

void AnimationManager::OnHelmetEffect(const std::string& name, const bool isEnable)
{
	if (!isEnable)
	{
		DisableHelmetEffect(name);

		return;
	}

	const auto tankIt = std::ranges::find_if(_turnBasedTankObjects, [&name](const AnimatedObject& tankObject)
	{
		return tankObject.type != AnimationType::Tank_Move || tankObject.name == name;
	});

	if (tankIt == _turnBasedTankObjects.end())
	{
		Log::Error("AnimationManager: no turn-based tank named " + name + " for the helmet effect");

		return;
	}

	//enable and update if exist
	const auto helmetIt = std::ranges::find_if(_autoAnimatedObjects, [&name](const AnimatedObject& animatedObject)
	{
		return animatedObject.type == AnimationType::Helmet_Effect && animatedObject.name.starts_with(name);
	});

	if (helmetIt == _autoAnimatedObjects.end())
	{
		CreateAnimation(AnimationType::Helmet_Effect, tankIt->rect, name);
		return;
	}

	helmetIt->markToDispose = false;
	helmetIt->rect.x = tankIt->rect.x;
	helmetIt->rect.y = tankIt->rect.y;
}

void AnimationManager::UpdateHelmetEffect(const std::string& name, const FPoint& pos)
{
	const auto it = std::ranges::find_if(_autoAnimatedObjects, [&name](const AnimatedObject& object)
	{
		return !object.markToDispose
			   && object.type == AnimationType::Helmet_Effect
			   && object.name.starts_with(name);
	});

	if (it != _autoAnimatedObjects.end())
	{
		//Update helmet animation position
		it->rect.x = pos.x;
		it->rect.y = pos.y;
	}
}

void AnimationManager::DisableTankAnimation(const std::string& name)
{
	auto matching = _turnBasedTankObjects | std::views::filter([&name](const AnimatedObject& object)
	{
		return object.name.ends_with(name);
	});

	std::ranges::for_each(matching, [](AnimatedObject& object) { object.markToDispose = true; });
}

void AnimationManager::DisableHelmetEffect(const std::string& name)
{
	auto matching = _autoAnimatedObjects | std::views::filter([&name](const AnimatedObject& object)
	{
		return !object.markToDispose
			   && object.type == AnimationType::Helmet_Effect
			   && object.name.starts_with(name);
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
