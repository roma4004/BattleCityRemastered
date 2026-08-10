#include "components/managers/AnimationManager.h"
#include "components/AnimatedObjects.h"
#include "components/SpawnEvents.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/ObjRectangle.h"
#include "enums/AnimationType.h"
#include "Point.h"
#include "utils/RandUtils.h"
#include <algorithm>
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
	_subs.push_back(_events->AddListener(_name, [this](const AnimationCreateEvent& event)
	{
		this->CreateAnimation(event.type, event.rect, event.name);
	}));

	_subs.push_back(_events->AddListener(_name, [this](const AnimationCreateTankExplosionEvent& event)
	{
		this->CreateAnimation(AnimationType::Tank_Explosion, event.rect, event.name);
	}));

	_subs.push_back(_events->AddListener(_name, [this](const AnimationCreateBulletExplosionEvent& event)
	{
		this->CreateAnimation(AnimationType::Bullet_Explosion, event.rect, event.name);
	}));

	_subs.push_back(_events->AddListener(_name, [this](const AnimationCreateTankEvent& event)
	{
		this->CreateAnimation(AnimationType::Tank_Animation, event.rect, event.name);
		this->OnHelmetEffect(event.name, true);
	}));

	_subs.push_back(_events->AddListener(_name, [this](const AnimationCreateWaterEvent& event)
	{
		this->CreateAnimation(AnimationType::Water_Animation, event.rect, "Water");
	}));

	_subs.push_back(_events->AddListener(_name, [this](const GameResetEvent&) { Reset(); }));
	_subs.push_back(_events->AddListener(_name, [this](const PostTickUpdateEvent&) { Update(); }));
	_subs.push_back(_events->AddListener(_name, [this](const AnimationTankUpdateEvent& event)
	{
		this->UpdateTank(event.name, event.pos, event.dir);
	}));
	_subs.push_back(_events->AddListener(_name, [this](const BonusHelmetAnimationChangeEvent& event)
	{
		this->OnHelmetEffect(event.name, event.isEnable);
	}));

	//TODO: draw explosion animation after others obstacle and tanks, maybe split explosions and other collections
	_subs.push_back(_events->AddListener(_name, [this](const DrawEvent&) { this->Draw(); }));
}

void AnimationManager::Reset()
{
	_autoAnimatedObjects.clear();
	_turnBasedTankObjects.clear();//NOTE: all tank_animation will be removed when tank died
	_autoAnimatedWaterObjects.clear();
}

void AnimationManager::CreateAnimation(const AnimationType type, const ObjRectangle rect, const std::string& name)
{
	switch (type)
	{
		case AnimationType::Spawn_Animation:
			Create("SpawnAnimation", rect, type, 3, 16, 20);
			break;
		case AnimationType::Bullet_Explosion:
			Create("BulletExplosion", rect, type, 3, 16, 20);
			break;
		case AnimationType::Tank_Explosion:
			DeleteTankAnimation(name);
			Create("TankExplosion", rect, type, 2, 32, 30);
			//TODO: should change limitOfFrame to 5?
			break;
		// case AnimationType::Bullet_Animation:
		// 	//NOTE: bullets are drawn via the texture manager, not animated here; speed 0 keeps UpdateFrame a no-op
		// 	Create("BulletAnimation", rect, type, 2, 16, 0);
		// 	break;
		case AnimationType::Water_Animation:
			Create("Water", rect, type, 16, 1, 20, true);
			break;
		case AnimationType::Helmet_Animation:
			Create(name + "HelmetAnimation", rect, type, 2, 16, 20, true);
			break;
		case AnimationType::Tank_Animation:
			Create(name, rect, type, 2, 16, 2, true);
			break;
	}
}

void AnimationManager::Create(const std::string& name, const ObjRectangle rect, const AnimationType type,
							  const int limitOfFrames, const int scale, const int animationSpeed,
							  const bool isInfinite)
{
	//NOTE: chose animation container for water if not then tanks, if not then other objects
	auto& target =
			type == AnimationType::Water_Animation
				? _autoAnimatedWaterObjects
				: type == AnimationType::Tank_Animation
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
		target.emplace_back(name, rect, type, limitOfFrames, scale, animationSpeed, isInfinite);
	}
}

void AnimationManager::Update()
{
	std::ranges::for_each(_autoAnimatedWaterObjects, UpdateFrame);
	std::ranges::for_each(_autoAnimatedObjects, UpdateFrame);
}

void AnimationManager::UpdateFrame(AnimatedObject& object)
{
	if (object.markToDispose || object.animationSpeed <= 0)
	{
		return;
	}

	if (++object.ticksSinceLastFrame % object.animationSpeed != 0)
	{
		return;
	}

	object.ticksSinceLastFrame = 0;
	if (++object.currentFrameIndex >= object.limitOfFrames)
	{
		if (object.isInfinite == false)
		{
			object.markToDispose = true;
		}

		object.currentFrameIndex = 0;
	}
}

void AnimationManager::UpdateTank(const std::string& name, const FPoint& pos, const Direction& dir)
{
	const auto it = std::ranges::find_if(_turnBasedTankObjects, [&name](const AnimatedObject& object)
	{
		return object.name.ends_with(name);
	});

	if (it == _turnBasedTankObjects.end())
	{
		return;
	}

	//Update tank animation position and dir
	it->rect.x = pos.x;
	it->rect.y = pos.y;
	it->dir = dir;

	UpdateFrame(*it);
	UpdateHelmetEffect(name, pos);
}

void AnimationManager::UpdateHelmetEffect(const std::string& name, const FPoint& pos)
{
	const auto it = std::ranges::find_if(_autoAnimatedObjects, [&name](const AnimatedObject& object)
	{
		return !object.markToDispose
			&& object.type == AnimationType::Helmet_Animation
			&& object.name.starts_with(name);
	});

	if (it != _autoAnimatedObjects.end())
	{
		//Update helmet animation position
		it->rect.x = pos.x;
		it->rect.y = pos.y;
	}
}

void AnimationManager::OnHelmetEffect(const std::string& name, const bool isEnable)
{
	if (!isEnable)
	{
		DeleteHelmetAnimation(name);
		return;
	}

	const auto tankIt = std::ranges::find_if(_turnBasedTankObjects, [&name](const AnimatedObject& tankObject)
	{
		return tankObject.type != AnimationType::Tank_Animation || tankObject.name == name;
	});

	if (tankIt == _turnBasedTankObjects.end())
	{
		std::cout << "AnimationManager [DEBUG] Fail to CreateHelmetAnimation: " << "name = " << name << '\n';
		return;
	}

	//enable and update if exist
	const auto helmetIt = std::ranges::find_if(_autoAnimatedObjects, [&name](const AnimatedObject& animatedObject)
	{
		return animatedObject.type == AnimationType::Helmet_Animation && animatedObject.name.starts_with(name);
	});

	if (helmetIt == _autoAnimatedObjects.end())
	{
		CreateAnimation(AnimationType::Helmet_Animation, tankIt->rect, name);
		return;
	}

	helmetIt->markToDispose = false;
	helmetIt->rect.x = tankIt->rect.x;
	helmetIt->rect.y = tankIt->rect.y;
}

void AnimationManager::DeleteTankAnimation(const std::string& name)
{
	auto matching = _turnBasedTankObjects | std::views::filter([&name](const AnimatedObject& object)
	{
		return object.name.ends_with(name);
	});

	std::ranges::for_each(matching, [](AnimatedObject& object) { object.markToDispose = true; });
}

void AnimationManager::DeleteHelmetAnimation(const std::string& name)
{
	auto matching = _autoAnimatedObjects | std::views::filter([&name](const AnimatedObject& object)
	{
		return !object.markToDispose
			&& object.type == AnimationType::Helmet_Animation
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

void AnimationManager::DrawObject(const AnimatedObject& object) const
{
	_events->EmitEvent(
			DrawAnimationEvent{.rect = object.rect,
							   .dir = object.dir,
							   .frame = object.currentFrameIndex,
							   .scale = object.scale,
							   .name = object.name});
}

void AnimationManager::Draw() const
{
	constexpr auto isEnabled = [](const AnimatedObject& object) { return !object.markToDispose; };

	for (const auto& object: _autoAnimatedWaterObjects | std::views::filter(isEnabled))
	{
		DrawObject(object);
	}

	for (const auto& object: _turnBasedTankObjects | std::views::filter(isEnabled))
	{
		DrawObject(object);
	}

	for (const auto& object: _autoAnimatedObjects | std::views::filter(isEnabled))
	{
		DrawObject(object);
	}
}
