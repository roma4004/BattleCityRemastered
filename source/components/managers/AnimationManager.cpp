#include "components/managers/AnimationManager.h"
#include "components/AnimatedObjects.h"
#include "components/SpawnEvents.h"
#include "components/events/AnimationRenderEvents.h"
#include "entities/ObjRectangle.h"
#include "enums/AnimationType.h"
#include "Point.h"
#include "utils/RandUtils.h"
#include <algorithm>

AnimationManager::AnimationManager(const std::shared_ptr<EventSystem>& events)
	: _events(events)
	, _gameMode{GameMode::Demo}
{
	_animatedObjects.reserve(100);
	_tankObjects.reserve(6);
	_waterObjects.reserve(10);

	Subscribe();
}

AnimationManager::~AnimationManager()
{
	Unsubscribe();
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

	_events->AddListener(
			"AnimationCreate", _name,
			[this](const AnimationCreateEvent& event)
			{
				this->CreateAnimation(event.type, event.rect, event.name);
			});
	_events->AddListener("Reset", _name, [this]() { Reset(); });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode) { SetGameMode(newGameMode); });
	_events->AddListener("PostTickUpdate", _name, [this](const double /*deltaTime*/) { Update(); });
	_events->AddListener(
			"AnimationTankUpdate", _name,
			[this](const AnimationTankUpdateEvent& event)
			{
				this->UpdateTank(event.name, event.pos, event.dir);
				this->UpdateHelmetEffect(event.name, event.pos);
			});
	_events->AddListener(
			"BonusHelmet_AnimationChange", _name,
			[this](const BonusHelmetAnimationChangeEvent& event)
			{
				this->OnHelmetEffect(event.name, event.isEnable);
			});

	_events->AddListener("TickUpdate", _name, [this](const double /*deltaTime*/) { this->AnimationSeqDisposer(); });
	//TODO: do not add new helmet animation if we already have for this tank
	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
}

void AnimationManager::SubscribeAsHost()
{
	_events->AddListener(
			"AnimationCreateTankExplosion", _name,
			[this](const AnimationCreateExplosionEvent& event)
			{
				this->CreateAnimation(AnimationType::Tank_Explosion, event.rect, event.name);
			});

	_events->AddListener(
			"AnimationCreateBulletExplosion", _name,
			[this](const AnimationCreateExplosionEvent& event)
			{
				this->CreateAnimation(AnimationType::Bullet_Explosion, event.rect, event.name);
			});

	//TODO: create client like subscription
	_events->AddListener(
			"AnimationCreateTank", _name,
			[this](const AnimationCreateTankEvent& event)
			{
				this->CreateAnimationTank(event.rect, event.name);
				this->OnHelmetEffect(event.name, true);
				//TODO: reuse animation
			});

	_events->AddListener(
			"AnimationCreateWater", _name,
			[this](const ObjRectangle rect)
			{
				this->CreateAnimationWater(rect);
			});
}

// void AnimationManager::SubscribeAsClient() {}

void AnimationManager::Unsubscribe() const { _events->RemoveAllListeners(_name); }

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
	_animatedObjects.clear();
	_tankObjects.clear();//NOTE: all tank_animation will be removed when tank died
	_waterObjects.clear();
}

void AnimationManager::CreateAnimation(const AnimationType type, const ObjRectangle rect, const std::string& name)
{
	switch (type)
	{
		case AnimationType::Spawn_Animation:
			Create("SpawnAnimation", rect, type, 3, 16);
			break;
		case AnimationType::Bullet_Explosion:
			Create("BulletExplosion", rect, type, 3, 16);
			break;
		case AnimationType::Tank_Explosion:
			DeleteTankAnimation(name);//TODO: fix tank explosion
			Create("TankExplosion", rect, type, 2, 32);
			//TODO: should change limitOfFrame to 5?
			break;
		case AnimationType::Bullet_Animation:
			Create("BulletAnimation", rect, type, 2, 16);
			break;
		default:
			break;
	}
}

void AnimationManager::CreateAnimationWater(const ObjRectangle rect)
{
	constexpr auto type = AnimationType::Water_Animation;
	constexpr bool isInfinite{true};
	_waterObjects.emplace_back("Water", rect, type, 16, 1, isInfinite);

	//TODO: extract to higher layer
	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(
				"ServerSend_AnimationCreate",
				ServerSendAnimationCreateEvent{.type = AnimationType::Water_Animation, .rect = rect, .name = "Water"});
	}
}

void AnimationManager::CreateHelmetAnimation(const ObjRectangle rect, const std::string& name)
{
	constexpr auto type{AnimationType::Helmet_Animation};
	constexpr bool isInfinite{true};
	constexpr int limitOfFrames{2};
	constexpr int scale{16};
	const std::string helmetAnimation{"HelmetAnimation"};
	_animatedObjects.emplace_back(name + helmetAnimation, rect, type, limitOfFrames, scale, isInfinite);
}

void AnimationManager::CreateAnimationTank(const ObjRectangle rect, std::string name)
{
	constexpr auto type{AnimationType::Tank_Animation};
	constexpr bool isInfinite{true};
	constexpr int limitOfFrames{2};
	constexpr int scale{16};
	_tankObjects.emplace_back(name, rect, type, limitOfFrames, scale, isInfinite);
}

void AnimationManager::Create(const std::string& name, const ObjRectangle rect, const AnimationType type,
							  const int limitOfFrames, const int scale, const bool isInfinite)
{
	_animatedObjects.emplace_back(name, rect, type, limitOfFrames, scale, isInfinite);

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_AnimationCreate",
						   ServerSendAnimationCreateEvent{.type = type, .rect = rect, .name = name});
	}
}

void AnimationManager::Update()
{
	for (auto& object: _waterObjects)
	{
		UpdateWaterFrame(object, 20);
	}

	for (auto& object: _animatedObjects)
	{
		if (object.markToDispose)
		{
			continue;
		}

		switch (object.type)
		{
			case AnimationType::Spawn_Animation:
			case AnimationType::Bullet_Explosion:
			case AnimationType::Helmet_Animation:
				UpdateFrame(object, 20);
				break;
			case AnimationType::Tank_Explosion:
				UpdateFrame(object, 30);
				break;
			//case AnimationType::Bullet_Animation:
			default:
				break;
		}
	}
}

void AnimationManager::UpdateFrame(AnimatedObject& object, const int animationSpeed)
{
	if (object.markToDispose == false
		&& ++object.elapsedFrames % animationSpeed == 0)
	{
		object.elapsedFrames = 0;
		if (++object.animationFrame >= object.limitOfFrames)
		{
			if (object.isInfinite == false)
			{
				object.markToDispose = true;
			}

			object.animationFrame = 0;
		}
	}
}

void AnimationManager::UpdateWaterFrame(AnimatedObject& object, const int animationSpeed)
{
	if (object.markToDispose == false
		&& ++object.elapsedFrames % animationSpeed == 0)
	{
		object.elapsedFrames = 0;
		if (++object.animationFrame == object.limitOfFrames)
		{
			object.animationFrame = 0;
		}
	}
}

void AnimationManager::UpdateTank(const std::string& name, const FPoint& pos, const Direction& dir)
{
	for (auto& object: _tankObjects)
	{
		if (object.name.ends_with(name))
		{
			//Update tank animation position and dir
			object.rect.x = pos.x;
			object.rect.y = pos.y;
			object.dir = dir;

			UpdateFrame(object, 2);
		}
	}
}

void AnimationManager::UpdateHelmetEffect(const std::string& name, const FPoint& pos)
{
	for (auto& object: _animatedObjects)
	{
		if (object.markToDispose == false
			&& object.type == AnimationType::Helmet_Animation
			&& object.name.starts_with(name))
		{
			//Update helmet animation position
			object.rect.x = pos.x;
			object.rect.y = pos.y;
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
		for (auto& tankObject: _tankObjects)
		{
			if (tankObject.type == AnimationType::Tank_Animation && tankObject.name != name)
			{
				continue;
			}

			//enable and update if exist
			for (auto& animatedObject: _animatedObjects)
			{
				if (animatedObject.type == AnimationType::Helmet_Animation && animatedObject.name.starts_with(name))
				{
					animatedObject.markToDispose = false;
					animatedObject.rect.x = tankObject.rect.x;
					animatedObject.rect.y = tankObject.rect.y;

					return;
				}
			}

			CreateHelmetAnimation(tankObject.rect, name);
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
	std::erase_if(_tankObjects, [&name](const auto& object)
	{
		return object.name.ends_with(name);
	});
}

void AnimationManager::DeleteHelmetAnimation(const std::string& name)
{
	for (auto& object: _animatedObjects)
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
void AnimationManager::AnimationSeqDisposer()//TODO: write correct disposer
{
	std::erase_if(_animatedObjects, [](const auto& object)
	{
		return object.markToDispose;
	});
}

void AnimationManager::DrawObject(const AnimatedObject& object) const
{
	const int currenAnimationFrame = {object.type == AnimationType::Water_Animation
										  ? -object.animationFrame//TODO: -animationFrame -> +animationFrame 
										  : object.animationFrame};//TODO: move this logic to UpdateFrameInfinite
	_events->EmitEvent(
			"DrawAnimation",
			DrawAnimationEvent{.rect = object.rect,
							   .dir = object.dir,
							   .frame = currenAnimationFrame,
							   .scale = object.scale,
							   .name = object.name});
}

void AnimationManager::Draw() const
{
	for (const auto& object: _waterObjects)
	{
		if (object.markToDispose)
		{
			continue;
		}

		DrawObject(object);
	}

	for (const auto& object: _tankObjects)
	{
		if (object.markToDispose)
		{
			continue;
		}

		DrawObject(object);
	}

	for (const auto& object: _animatedObjects)
	{
		if (object.markToDispose)
		{
			continue;
		}

		DrawObject(object);
	}
}

//TODO: add reuse flow for explosions like bullet pool
