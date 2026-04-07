#include "components/managers/AnimationManager.h"
#include "components/AnimatedObjects.h"
#include "entities/ObjRectangle.h"
#include "entities/pawns/Tank.h"
#include "enums/AnimationType.h"
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
			[this](const AnimationType& type, const ObjRectangle& rect, const std::string& objName)
			{
				this->CreateAnimation(type, rect, objName);
			});
	_events->AddListener("Reset", _name, [this]() { Reset(); });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode) { SetGameMode(newGameMode); });
	_events->AddListener("PostTickUpdate", _name, [this](const double /*deltaTime*/) { Update(); });
	_events->AddListener("AnimationTankUpdate", _name,
						 [this](const std::string& objName, const ObjRectangle& rect, const Direction& dir)
						 {
							 UpdateTank(objName, rect, dir);//TODO:replicate this or recheck
						 });
	_events->AddListener("PreTickUpdate", _name, [this](const double /*deltaTime*/) { this->AnimationSeqDisposer(); });
}

void AnimationManager::SubscribeAsHost()
{
	_events->AddListener(
			"AnimationCreateTankExplosion", _name,
			[this](const ObjRectangle rect, const std::string& objName)
			{
				this->CreateAnimation(AnimationType::Tank_Explosion, rect, objName);
			});

	_events->AddListener(
			"AnimationCreateBulletExplosion", _name,
			[this](const ObjRectangle rect, const std::string& objName)
			{
				this->CreateAnimation(AnimationType::Bullet_Explosion, rect, objName);
			});

	//TODO: create client like subscription
	_events->AddListener(
			"AnimationCreateTank", _name,
			[this](const std::weak_ptr<Tank>& tank)
			{
				this->CreateAnimationTank(tank);
			});

	_events->AddListener(
			"AnimationCreateWater", _name,
			[this](const ObjRectangle rect)
			{
				this->CreateAnimationWater(rect);
			});
}

// void AnimationManager::SubscribeAsClient() {}

void AnimationManager::Unsubscribe() const
{
	if (_gameMode == GameMode::PlayAsClient)
	{
		// UnsubscribeAsClient();
	}
	else
	{
		UnsubscribeAsHost();
	}

	_events->RemoveListener("AnimationCreate", _name);
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("GameModeChangedTo", _name);
	_events->RemoveListener("TickUpdate", _name);
	_events->RemoveListener("AnimationTankUpdate", _name);
	_events->RemoveListener("PostTickUpdate", _name);
}

// void AnimationManager::UnsubscribeAsClient() const {}

void AnimationManager::UnsubscribeAsHost() const
{
	_events->RemoveListener("AnimationCreateTankExplosion", _name);
	_events->RemoveListener("AnimationCreateBulletExplosion", _name);
	_events->RemoveListener("AnimationCreateTank", _name);
	_events->RemoveListener("AnimationCreateWater", _name);
}

void AnimationManager::SetGameMode(const GameMode newGameMode)
{
	_gameMode = newGameMode;
	_animatedObjects.clear();
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

void AnimationManager::CreateAnimation(const AnimationType type, const ObjRectangle rect, const std::string& objName)
{
	switch (type)
	{
		case AnimationType::Spawn_Animation:
			Create("SpawnAnimation", rect, type, 3, 16, objName);
			break;
		case AnimationType::Bullet_Explosion:
			Create("BulletExplosion", rect, type, 3, 16, objName);
			break;
		case AnimationType::Tank_Explosion:
			DeleteTankAnimation(objName);//TODO: fix tank explosion
			Create("TankExplosion", rect, type, 2, 32, objName);
			//TODO: should change limitOfFrame to 5?
			break;
		case AnimationType::Helmet_Animation:
			Create("HelmetAnimation", rect, type, 2, 16, objName);
			break;
		case AnimationType::Bullet_Animation:
			Create("BulletAnimation", rect, type, 2, 16, objName);
			break;
		default:
			break;
	}
}

void AnimationManager::CreateAnimationWater(const ObjRectangle rect)
{
	constexpr unsigned int color{0};
	constexpr auto type = AnimationType::Water_Animation;
	constexpr bool isInfinite{true};
	std::string objName = "Water";
	_waterObjects.emplace_back(objName, rect, type, _events, 16, 1, objName, color, isInfinite);

	//TODO: extract to higher layer
	if (_gameMode == GameMode::PlayAsHost)
	{
		this->_events->EmitEvent("ServerSend_AnimationCreate", AnimationType::Water_Animation, rect, "Water");
	}
}

void AnimationManager::CreateAnimationTank(const std::weak_ptr<Tank>& tank)
{
	const auto tankLck = tank.lock();
	if (!tankLck)
	{
		return;//TODO: add assert in this case
	}

	const ObjRectangle rect = tankLck->GetRect();
	const std::string objName(tankLck->GetName());
	const unsigned int color = tankLck->GetColor();
	constexpr auto type = AnimationType::Tank_Animation;
	const std::string name = "TankAnimation";
	constexpr bool isInfinite{true};
	_tankObjects.emplace_back(name, rect, type, _events, 2, 16, objName, color, isInfinite);

	if (_gameMode == GameMode::PlayAsHost)
	{
		this->_events->EmitEvent("ServerSend_AnimationCreate", type, rect, objName);
	}
}

void AnimationManager::Create(const std::string& name, const ObjRectangle rect, const AnimationType type,
							  const int limitOfFrames, const int scale, const std::string& objName,
							  const bool isInfinite)
{
	constexpr int placeholderWhiteColor = 0xffffff;
	_animatedObjects.emplace_back(name, rect, type, _events, limitOfFrames, scale, objName, placeholderWhiteColor,
								  isInfinite);

	if (_gameMode == GameMode::PlayAsHost)
	{
		this->_events->EmitEvent("ServerSend_AnimationCreate", type, rect, objName);
	}
}

void AnimationManager::Update()
{
	for (auto& animatedObj: _waterObjects)
	{
		UpdateFrameInfinite(animatedObj, 20);
	}

	for (auto& animatedObj: _animatedObjects)
	{
		switch (animatedObj.type)
		{
			case AnimationType::Spawn_Animation:
			case AnimationType::Bullet_Explosion:
				UpdateFrame(animatedObj, 20);
				break;
			case AnimationType::Tank_Explosion:
				UpdateFrame(animatedObj, 30);
				break;
			//case AnimationType::Helmet_Animation:
			//case AnimationType::Bullet_Animation:
			default:
				break;
		}
	}
}

void AnimationManager::UpdateFrame(AnimatedObject& obj, const int animationSpeed)
{
	if (obj.markToDispose == false
		&& ++obj.elapsedFrames % animationSpeed == 0)
	{
		obj.elapsedFrames = 0;
		if (++obj.animationFrame >= obj.limitOfFrames)
		{
			if (obj.isInfinite == false)
			{
				obj.markToDispose = true;
			}
			obj.animationFrame = 0;
		}
	}
}

void AnimationManager::UpdateFrameInfinite(AnimatedObject& obj, const int animationSpeed)
{
	if (obj.markToDispose == false
		&& ++obj.elapsedFrames % animationSpeed == 0)
	{
		obj.elapsedFrames = 0;
		if (++obj.animationFrame == obj.limitOfFrames)
		{
			obj.animationFrame = 0;
		}
	}
}

void AnimationManager::UpdateTank(const std::string& objName, const ObjRectangle& rect, const Direction& dir)
{
	for (AnimatedObject& animObj: _tankObjects)
	{
		if (animObj.objName == objName)
		{
			//Update tank animation position and dir
			animObj.rect.x = rect.x;
			animObj.rect.y = rect.y;
			animObj.dir = dir;

			UpdateFrame(animObj, 20);
			return;
		}
	}
}

void AnimationManager::DisableTankAnimation(const std::string& objName)
{
	for (AnimatedObject& animObj: _tankObjects)
	{
		if (animObj.objName == objName)
		{
			animObj.markToDispose = true;
			return;
		}
	}
}

void AnimationManager::DeleteTankAnimation(const std::string& objName)
{
	std::erase_if(_tankObjects, [&objName](const auto& animObj)
	{
		return animObj.objName == objName;
	});
}

// NOTE: how it works
// it = [0][1][2][3][4][5][6]
// mark  t  t  f  t  f  t  f

// it = [2][4][6] [0][1][3][5]
//  cut here     |
void AnimationManager::AnimationSeqDisposer()//TODO: write correct disposer
{
	std::erase_if(_animatedObjects, [](const auto& obj)
	{
		return obj.markToDispose;
	});
}

//TODO: add reuse flow for explosions like bullet pool
