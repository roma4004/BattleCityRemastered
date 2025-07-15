#include "components/managers/AnimationManager.h"
#include "components/AnimatedObjects.h"
#include "entities/ObjRectangle.h"
#include "enums/AnimationType.h"
#include "utils/RandUtils.h"
#include <algorithm>

AnimationManager::AnimationManager(std::shared_ptr<EventSystem> events)
	: _events(std::move(events)),
	  _gameMode{GameMode::Demo}
{
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
	_events->AddListener("AnimationTankUpdate", _name, [this](const buuid& uuid) { UpdateTank(uuid); });

	//_events->AddListener("AnimationWaterUpdate", _name, [this]() { UpdateWater(); });
}

void AnimationManager::SubscribeAsHost()
{
	_events->AddListener(
			"AnimationCreate", _name,
			[this](const AnimationType type, const ObjRectangle rect, const buuid& uuid)
			{
				CreateAnimation(type, rect, uuid);
			});
}

void AnimationManager::SubscribeAsClient()//TODO: merge with host?
{
	_events->AddListener(
			"ClientReceived_AnimationCreate", _name,
			[this](const AnimationType type, const ObjRectangle rect, const buuid& uuid)
			{
				CreateAnimation(type, rect, uuid);
			});
}

void AnimationManager::Unsubscribe() const
{
	_gameMode == GameMode::PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("GameModeChangedTo", _name);
	_events->RemoveListener("AnimationUpdate", _name);
	_events->RemoveListener("AnimationTankUpdate", _name);
}

void AnimationManager::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_AnimationCreate", _name);
}

void AnimationManager::UnsubscribeAsHost() const
{
	_events->RemoveListener("AnimationCreate", _name);
}

void AnimationManager::SetGameMode(const GameMode newGameMode)
{
	_gameMode = newGameMode;
	_animatedObjects.clear();
	_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
}

void AnimationManager::Reset() { _animatedObjects.clear(); }

void AnimationManager::CreateAnimation(const AnimationType type, const ObjRectangle rect, buuid uuid)
{
	switch (type)
	{
		//TODO: investigate
		// when one players mode we don't see black animation instead of spawn anumation
		// when two players mode we see spawn animation under first player and above second player
		// when for coop mode we see spawn animation under first and second player
		case AnimationType::Spawn_Animation: //TODO: maybe uniq id for each explosion for reusing bullet id
			Create("SpawnAnimation", type, rect, std::move(uuid), 3);
			//TODO:add is loop flag or separated container for expired explosion
			break;
		case AnimationType::Bullet_Explosion:
			Create("BulletExplosion", type, rect, std::move(uuid), 3);//TODO: should change to 1?
			break;
		case AnimationType::Tank_Explosion:
			Create("TankExplosion", type, rect, std::move(uuid), 2);//TODO: should change to 1?
			break;
		case AnimationType::Water_Animation:
			Create("WaterAnimation", type, rect, std::move(uuid), 16);
			break;
		case AnimationType::Helmet_Animation:
			Create("HelmetAnimation", type, rect, std::move(uuid), 2);
			break;
		case AnimationType::Tank_Animation:
			Create("TankAnimation", type, rect, std::move(uuid), 1);
			break;
		case AnimationType::Bullet_Animation:
			Create("BulletAnimation", type, rect, std::move(uuid), 2);
			break;
	}
}

void AnimationManager::Create(const std::string& name, const AnimationType type, const ObjRectangle rect,
                              const buuid& uuid, const int limitOfFrames)
{
	_animatedObjects.emplace_back(name, rect, type, _events, uuid, _gameMode, limitOfFrames);
}

void AnimationManager::Update()
{
	UpdateWaterAnimation();
	for (auto& passport: _animatedObjects)
	{
		switch (passport.type)
		{
			case AnimationType::Spawn_Animation:
			case AnimationType::Bullet_Explosion:
				UpdateFrame(passport, 20);
				break;
			case AnimationType::Tank_Explosion:
				UpdateFrame(passport, 30);
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
			obj.animationFrame = 0;
		}
	}
}

void AnimationManager::UpdateTank(const buuid& uuid)
{
	for (auto& passport: _animatedObjects)
	{
		if (passport.GetUuid() == uuid && passport.type == AnimationType::Tank_Animation)
		{
			UpdateFrame(passport, 20);
			return;
		}
	}
}

void AnimationManager::UpdateWaterAnimation()
{
	UpdateFrame(_waterAnimationPassport, 20);
}

int AnimationManager::GetWaterFrame() const
{
	return _waterAnimationPassport.animationFrame;
}

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
void AnimationManager::AnimationSeqDisposer()
{
	std::erase_if(_animatedObjects, [](const auto& obj) { return obj.markToDispose; });
}
