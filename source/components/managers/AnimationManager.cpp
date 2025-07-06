#include "components/managers/AnimationManager.h"
#include "components/AnimatedObjects.h"
#include "entities/ObjRectangle.h"
#include "enums/AnimationType.h"
#include "network/Client.h"
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

	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;

		_animatedObjects.clear();
		_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
	});

	_events->AddListener("AnimationUpdate", _name, [this]() { Update(); });
	_events->AddListener<const buuid&>("AnimationTankUpdate", _name, [this](const buuid& uuid) { UpdateTank(uuid); });

	//_events->AddListener("AnimationWaterUpdate", _name, [this]() { UpdateWater(); });
}

void AnimationManager::SubscribeAsHost()
{
	_events->AddListener<const AnimationType, const ObjRectangle&, const buuid&>(
			"AnimationCreate", _name,
			[this](const AnimationType type, const ObjRectangle& rect, const buuid& uuid)
			{
				switch (type)
				{
					case AnimationType::Spawn_Animation:
						Create("SpawnAnimation", type, rect, uuid, 3);
						break;
					case AnimationType::Bullet_Explosion:
						Create("BulletExplosion", type, {rect.x, rect.y, rect.w * 4, rect.h * 4}, uuid, 3);
						break;
					case AnimationType::Tank_Explosion:
						Create("TankExplosion", type, {rect.x, rect.y, rect.w * 4, rect.h * 4}, uuid, 2);
						break;
					case AnimationType::Helmet_Animation:
						Create("HelmetAnimation", type, rect, uuid, 2);
						break;
					case AnimationType::Tank_Animation:
						Create("TankAnimation", type, rect, uuid, 2);
						break;
					case AnimationType::Bullet_Animation:
						Create("BulletAnimation", type, rect, uuid, 2);
						break;
					default:
						break;
				}
			});
}

void AnimationManager::SubscribeAsClient()
{
	_events->AddListener<const AnimationType, const ObjRectangle&, const buuid&>(
			"ClientReceived_AnimationCreate", _name,
			[this](const AnimationType type, const ObjRectangle& rect, const buuid& uuid)
			{
				switch (type)
				{
					case AnimationType::Spawn_Animation:
						Create("SpawnAnimation", type, rect, uuid, 3);
						break;
					case AnimationType::Bullet_Explosion:
						Create("BulletExplosion", type, rect, uuid, 3);
						break;
					case AnimationType::Tank_Explosion:
						Create("TankExplosion", type, rect, uuid, 2);
						break;
					case AnimationType::Water_Animation:
						Create("WaterAnimation", type, rect, uuid, 16);
						break;
					case AnimationType::Helmet_Animation:
						Create("HelmetAnimation", type, rect, uuid, 2);
						break;
					case AnimationType::Tank_Animation:
						Create("TankAnimation", type, rect, uuid, 1);
						break;
					case AnimationType::Bullet_Animation:
						Create("BulletAnimation", type, rect, uuid, 2);
						break;
				}
			});
}

void AnimationManager::Unsubscribe() const
{
	_gameMode == GameMode::PlayAsClient ? UnsubscribeAsClient() : UnsubscribeAsHost();
	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);
	_events->RemoveListener("AnimationUpdate", _name);
	_events->RemoveListener<const buuid&>("AnimationTankUpdate", _name);
}

void AnimationManager::UnsubscribeAsClient() const
{
	_events->RemoveListener<const AnimationType, const ObjRectangle&, const buuid&>(
			"ClientReceived_AnimationCreate", _name);
}

void AnimationManager::UnsubscribeAsHost() const
{
	_events->RemoveListener<const AnimationType, const ObjRectangle&, const buuid&>("AnimationCreate", _name);
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
	if (!obj.markToDispose)
	{
		if (++obj.elapsedFrames % animationSpeed == 0)
		{
			obj.elapsedFrames = 0;
			if (++obj.animationFrame == obj.limitOfFrames)
			{
				obj.animationFrame = 0;
			}
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

int AnimationManager::GetFrame(const buuid& uuid, const AnimationType type) const
{
	for (auto& obj: _animatedObjects)
	{
		if (obj.GetUuid() == uuid && obj.type == type)
		{
			return obj.animationFrame;
		}
	}

	return 0;
}

void AnimationManager::AnimationSeqDisposer()
{
	const auto it = std::ranges::remove_if(_animatedObjects, [](const auto& obj)
	{
		return obj.markToDispose;
	}).begin();

	//it=	[0]	[1]	[2]	[3]	[4]	[5]	[6]
	//mark	t	t	f	t	f	t	f

	//it = [2][4][6] [0][1][3][5]
	//  cut here    |

	_animatedObjects.erase(it, _animatedObjects.end());
}
