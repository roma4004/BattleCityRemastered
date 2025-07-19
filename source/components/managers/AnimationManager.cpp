#include "components/managers/AnimationManager.h"
#include "components/AnimatedObjects.h"
#include "entities/ObjRectangle.h"
#include "entities/pawns/Tank.h"
#include "enums/AnimationType.h"
#include "utils/RandUtils.h"
#include "utils/UuidUtils.h"
#include <algorithm>

AnimationManager::AnimationManager(std::shared_ptr<EventSystem> events)
	: _events(std::move(events)),
	  _gameMode{GameMode::Demo}
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
	_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : SubscribeAsHost();

	_events->AddListener("Reset", _name, [this]() { Reset(); });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode) { SetGameMode(newGameMode); });
	_events->AddListener("AnimationUpdate", _name, [this]() { Update(); });
	_events->AddListener("AnimationTankUpdate", _name, [this](const std::string& uuid) { UpdateTank(uuid); });
	_events->AddListener("DisposeStage", _name, [this]() { this->AnimationSeqDisposer(); });
}

void AnimationManager::SubscribeAsHost()
{
	_events->AddListener(
			"AnimationCreate", _name,
			[this](const AnimationType type, const ObjRectangle rect, const std::string& name)
			{
				this->CreateAnimation(type, rect, name);
			});

	_events->AddListener("AnimationCreateTank", _name, [this](const BaseObj* obj)
	{
		this->CreateAnimationTank(obj->GetRect(), UuidUtils::GetRandomUuid(), std::string(obj->GetName()), obj);
	});
	_events->AddListener("AnimationCreateWater", _name, [this](const ObjRectangle rect)
	{
		this->CreateAnimationWater(rect);
	});
}

void AnimationManager::SubscribeAsClient()//TODO: merge with host?
{
	_events->AddListener(
			"ClientReceived_AnimationCreate", _name,
			[this](const AnimationType type, const ObjRectangle rect, const std::string& name)
			//TODO: change command add field add , const std::string& objName
			{
				CreateAnimation(type, rect, name);
			});
}

void AnimationManager::Unsubscribe() const
{
	_gameMode == GameMode::PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("GameModeChangedTo", _name);
	_events->RemoveListener("AnimationUpdate", _name);
	_events->RemoveListener("AnimationTankUpdate", _name);
	_events->RemoveListener("DisposeStage", _name);
}

void AnimationManager::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_AnimationCreate", _name);
}

void AnimationManager::UnsubscribeAsHost() const
{
	_events->RemoveListener("AnimationCreate", _name);
	_events->RemoveListener("AnimationCreateTank", _name);
	_events->RemoveListener("AnimationCreateWater", _name);
}

void AnimationManager::SetGameMode(const GameMode newGameMode)
{
	_gameMode = newGameMode;
	_animatedObjects.clear();
	_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
}

void AnimationManager::Reset()
{
	_animatedObjects.clear();
	// _tankObjects.clear(); //NOTE: all tank_animation will be removed when tank died
	_waterObjects.clear();
}

void AnimationManager::CreateAnimation(const AnimationType type, const ObjRectangle rect, const std::string& name)
{
	const auto uuid = UuidUtils::GetRandomUuid();
	switch (type)
	{
		case AnimationType::Spawn_Animation:
			Create("SpawnAnimation", type, rect, uuid, 3, 16, std::move(name));
			break;
		case AnimationType::Bullet_Explosion:
			Create("BulletExplosion", type, rect, uuid, 3, 16, std::move(name));
			break;
		case AnimationType::Tank_Explosion: //TODO: fix tank explosion
			DeleteAnimation(name);
			Create("TankExplosion", type, rect, uuid, 2, 32, std::move(name));//TODO: should change limitOfFrame to 5?
			break;
		case AnimationType::Helmet_Animation:
			Create("HelmetAnimation", type, rect, uuid, 2, 16, std::move(name));
			break;
		case AnimationType::Bullet_Animation:
			Create("BulletAnimation", type, rect, uuid, 2, 16, std::move(name));
			break;
		default:
			break;
	}
}

void AnimationManager::DeleteAnimation(const std::string& objName)
{
	std::erase_if(_tankObjects, [&objName](const auto& animObj)
	{
		return animObj.objName == objName;
	});
}

void AnimationManager::CreateAnimationWater(const ObjRectangle rect)
{
	_waterObjects.emplace_back(rect, _events, 16);
}

void AnimationManager::CreateAnimationTank(const ObjRectangle rect, buuid uuid, std::string name, const BaseObj* obj)
{
	_tankObjects.emplace_back(rect, _events, std::move(uuid), _gameMode, 2, 16, std::move(name), obj);
}

void AnimationManager::Create(const std::string& name, const AnimationType type, const ObjRectangle rect,
                              buuid uuid, const int limitOfFrames, const int scale, std::string objName)
{
	_animatedObjects.emplace_back(name, rect, type, _events, std::move(uuid), _gameMode, limitOfFrames, scale, objName);
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
		if (++obj.animationFrame == obj.limitOfFrames)
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

void AnimationManager::UpdateTank(const std::string& objName)//TODO: replace uuid with name or filter by parentUuid
{
	for (AnimatedObject& animObj: _tankObjects)
	{
		if (animObj.objName == objName)
		{
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

//deprecated
int AnimationManager::GetFrame(buuid uuid, const AnimationType type) const
{
	auto predicate = [uuid = std::move(uuid), type](const AnimatedObject& obj)
	{
		return obj.GetUuid() == uuid && obj.type == type;
	};
	const auto it = std::ranges::find_if(_animatedObjects, predicate);

	return it != _animatedObjects.end() ? it->animationFrame : 0;
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

	// for (auto it = _animatedObjects.begin(); it != _animatedObjects.end(); ++it)
	// {
	// 	_animatedObjects.erase(it);
	// }
}

//TODO: add reuse flow for explosions like bullet pool
