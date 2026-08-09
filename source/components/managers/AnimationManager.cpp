#include "components/managers/AnimationManager.h"
#include "components/AnimatedObjects.h"
#include "components/SpawnEvents.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/ObjRectangle.h"
#include "enums/AnimationType.h"
#include "Point.h"
#include "utils/RandUtils.h"

AnimationManager::AnimationManager(const std::shared_ptr<EventSystem>& events)
	: _events(events)
	, _gameMode{GameMode::Demo}
{
	_autoAnimatedObjects.reserve(100);
	_turnBasedTankObjects.reserve(6);
	_autoAnimatedWaterObjects.reserve(10);

	Subscribe();
}

void AnimationManager::Subscribe()
{
	if (_gameMode == GameMode::PlayAsClient)
	{
		//SubscribeAsClient();
	}
	else
	{
		SubscribeAsHost();
	}

	_subs.push_back(_events->AddListener(_name, [this](const AnimationCreateEvent& event)
	{
		this->CreateAnimation(event.type, event.rect, event.name);
	}));
	_subs.push_back(_events->AddListener(_name, [this](const GameResetEvent&) { Reset(); }));
	_subs.push_back(
			_events->AddListener(_name, [this](const GameModeChangedToEvent& event) { SetGameMode(event.mode); }));
	_subs.push_back(_events->AddListener(_name, [this](const PostTickUpdateEvent&) { Update(); }));
	_subs.push_back(_events->AddListener(_name, [this](const AnimationTankUpdateEvent& event)
	{
		this->UpdateTank(event.name, event.pos, event.dir);
	}));
	_subs.push_back(_events->AddListener(_name, [this](const BonusHelmetAnimationChangeEvent& event)
	{
		this->OnHelmetEffect(event.name, event.isEnable);
	}));

	_subs.push_back(_events->AddListener(_name, [this](const TickUpdateEvent&) { this->AnimationSeqDisposer(); }));
	//TODO: do not add new helmet animation if we already have for this tank
	_subs.push_back(_events->AddListener(_name, [this](const DrawEvent&) { this->Draw(); }));
}

void AnimationManager::SubscribeAsHost()
{
	_subs.push_back(_events->AddListener(_name, [this](const AnimationCreateTankExplosionEvent& event)
	{
		this->CreateAnimation(AnimationType::Tank_Explosion, event.rect, event.name);
	}));

	_subs.push_back(_events->AddListener(_name, [this](const AnimationCreateBulletExplosionEvent& event)
	{
		this->CreateAnimation(AnimationType::Bullet_Explosion, event.rect, event.name);
	}));

	//TODO: create client like subscription
	_subs.push_back(_events->AddListener(_name, [this](const AnimationCreateTankEvent& event)
	{
		this->CreateAnimation(AnimationType::Tank_Animation, event.rect, event.name);
		this->OnHelmetEffect(event.name, true);
		//TODO: reuse animation
	}));

	_subs.push_back(_events->AddListener(_name, [this](const AnimationCreateWaterEvent& event)
	{
		this->CreateAnimation(AnimationType::Water_Animation, event.rect, "Water");
	}));
}

// void AnimationManager::SubscribeAsClient() {}

void AnimationManager::SetGameMode(const GameMode newGameMode)
{
	_gameMode = newGameMode;
	// if (_gameMode == GameMode::PlayAsClient)
	// {
	// 	SubscribeAsClient();
	// }
	// else
	// {
	// 	UnsubscribeAsClient();
	// }
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
		{
			//NOTE: isLocallySimulated=true: each client has its own helmet animation locally from its own tank
			constexpr bool isLocallySimulated{true};
			Create(name + "HelmetAnimation", rect, type, 2, 16, 20, true, isLocallySimulated);
		}
		break;
		case AnimationType::Tank_Animation:
			//NOTE: isLocallySimulated=true: each client has its own tank animation from its own spawn-enabled trigger
			constexpr bool isLocallySimulated{true};
			Create(name, rect, type, 2, 16, 2, true, isLocallySimulated);
			break;
	}
}

void AnimationManager::Create(const std::string& name, const ObjRectangle rect, const AnimationType type,
							  const int limitOfFrames, const int scale, const int animationSpeed,
							  const bool isInfinite, const bool isLocallySimulated)
{
	//NOTE: chose animation container for water if not then tanks, if not then other objects
	auto& target =
			type == AnimationType::Water_Animation
				? _autoAnimatedWaterObjects
				: type == AnimationType::Tank_Animation
				? _turnBasedTankObjects
				: _autoAnimatedObjects;
	target.emplace_back(name, rect, type, limitOfFrames, scale, animationSpeed, isInfinite);

	if (!isLocallySimulated && _gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutAnimationCreateEvent{.type = type, .rect = rect, .name = name});
	}
}

void AnimationManager::Update()
{
	for (auto& object: _autoAnimatedWaterObjects)
	{
		UpdateFrame(object);
	}

	for (auto& object: _autoAnimatedObjects)
	{
		UpdateFrame(object);
	}
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
	for (auto& object: _turnBasedTankObjects)
	{
		if (object.name.ends_with(name))
		{
			//Update tank animation position and dir
			object.rect.x = pos.x;
			object.rect.y = pos.y;
			object.dir = dir;

			UpdateFrame(object);
			UpdateHelmetEffect(name, pos);
			break;
		}
	}
}

void AnimationManager::UpdateHelmetEffect(const std::string& name, const FPoint& pos)
{
	for (auto& object: _autoAnimatedObjects)
	{
		if (object.markToDispose == false
			&& object.type == AnimationType::Helmet_Animation
			&& object.name.starts_with(name))
		{
			//Update helmet animation position
			object.rect.x = pos.x;
			object.rect.y = pos.y;
			break;
		}
	}
}

void AnimationManager::OnHelmetEffect(const std::string& name, const bool isEnable)
{
	if (!isEnable)
	{
		DeleteHelmetAnimation(name);
	}
	else
	{
		for (auto& tankObject: _turnBasedTankObjects)
		{
			if (tankObject.type == AnimationType::Tank_Animation && tankObject.name != name)
			{
				continue;
			}

			//enable and update if exist
			for (auto& animatedObject: _autoAnimatedObjects)
			{
				if (animatedObject.type == AnimationType::Helmet_Animation && animatedObject.name.starts_with(name))
				{
					animatedObject.markToDispose = false;
					animatedObject.rect.x = tankObject.rect.x;
					animatedObject.rect.y = tankObject.rect.y;

					return;
				}
			}

			CreateAnimation(AnimationType::Helmet_Animation, tankObject.rect, name);
			return;
		}

		std::cout << "AnimationManager [DEBUG] Fail to CreateHelmetAnimation: " << "name = " << name << '\n';
	}
}

// void AnimationManager::DisableTankAnimation(const std::string& name)//TODO: add reuse flow for animation
// {
// 	for (auto& object: _tankObjects)
// 	{
// 		if (object.name == name)
// 		{
// 			object.markToDispose = true;
// 			return;
// 		}
// 	}
// }

void AnimationManager::DeleteTankAnimation(const std::string& name)
{
	std::erase_if(_turnBasedTankObjects, [&name](const auto& object)
	{
		return object.name.ends_with(name);
	});
}

void AnimationManager::DeleteHelmetAnimation(const std::string& name)
{
	for (auto& object: _autoAnimatedObjects)
	{
		if (object.markToDispose == false
			&& object.type == AnimationType::Helmet_Animation
			&& object.name.starts_with(name))
		{
			object.markToDispose = true;
		}
	}
}

// NOTE: how it works
// it = [0][1][2][3][4][5][6]
// mark  t  t  f  t  f  t  f

// it = [2][4][6] [0][1][3][5]
//  cut here     |
void AnimationManager::AnimationSeqDisposer()
{
	std::erase_if(_autoAnimatedObjects, [](const auto& object)
	{
		return object.markToDispose;
	});
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
	for (const auto& object: _autoAnimatedWaterObjects)
	{
		if (object.markToDispose)
		{
			continue;
		}

		DrawObject(object);
	}

	for (const auto& object: _turnBasedTankObjects)
	{
		if (object.markToDispose)
		{
			continue;
		}

		DrawObject(object);
	}

	for (const auto& object: _autoAnimatedObjects)
	{
		if (object.markToDispose)
		{
			continue;
		}

		DrawObject(object);
	}
}

//TODO: add reuse flow for explosions like bullet pool
