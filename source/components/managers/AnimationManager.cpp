#include "components/managers/AnimationManager.h"
#include "components/AnimatedObjects.h"
#include "entities/ObjRectangle.h"
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
			[this](const AnimationType& type, const ObjRectangle& rect, const std::string& name)
			{
				this->CreateAnimation(type, rect, name);
			});
	_events->AddListener("Reset", _name, [this]() { Reset(); });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode) { SetGameMode(newGameMode); });
	_events->AddListener("PostTickUpdate", _name, [this](const double /*deltaTime*/) { Update(); });
	_events->AddListener(
			"AnimationTankUpdate", _name,
			[this](const std::string& name, const ObjRectangle& rect, const Direction& dir)
			{
				UpdateTank(name, rect, dir);
			});
	_events->AddListener("PreTickUpdate", _name, [this](const double /*deltaTime*/) { this->AnimationSeqDisposer(); });

	_events->AddListener("Draw", _name, [this]() { this->Draw(); });
}

void AnimationManager::SubscribeAsHost()
{
	_events->AddListener(
			"AnimationCreateTankExplosion", _name,
			[this](const ObjRectangle rect, const std::string& name)
			{
				this->CreateAnimation(AnimationType::Tank_Explosion, rect, name);
			});

	_events->AddListener(
			"AnimationCreateBulletExplosion", _name,
			[this](const ObjRectangle rect, const std::string& name)
			{
				this->CreateAnimation(AnimationType::Bullet_Explosion, rect, name);
			});

	//TODO: create client like subscription
	_events->AddListener(
			"AnimationCreateTank", _name,
			[this](const ObjRectangle rect, const std::string& name)
			{
				this->CreateAnimationTank(rect, name);
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
		case AnimationType::Helmet_Animation:
			Create("HelmetAnimation", rect, type, 2, 16);
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
		this->_events->EmitEvent("ServerSend_AnimationCreate", AnimationType::Water_Animation, rect, "Water");
	}
}

void AnimationManager::CreateAnimationTank(const ObjRectangle rect, const std::string name)
{
	constexpr auto type{AnimationType::Tank_Animation};
	constexpr bool isInfinite{true};
	_tankObjects.emplace_back(name, rect, type, 2, 16, isInfinite);

	if (_gameMode == GameMode::PlayAsHost)
	{
		this->_events->EmitEvent("ServerSend_AnimationCreate", type, rect, name);
	}
}

void AnimationManager::Create(const std::string& name, const ObjRectangle rect, const AnimationType type,
							  const int limitOfFrames, const int scale, const bool isInfinite)
{
	_animatedObjects.emplace_back(name, rect, type, limitOfFrames, scale, isInfinite);

	if (_gameMode == GameMode::PlayAsHost)
	{
		this->_events->EmitEvent("ServerSend_AnimationCreate", type, rect, name);
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
		switch (object.type)
		{
			case AnimationType::Spawn_Animation:
			case AnimationType::Bullet_Explosion:
				UpdateFrame(object, 20);
				break;
			case AnimationType::Tank_Explosion:
				UpdateFrame(object, 30);
				break;
			//case AnimationType::Helmet_Animation:
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

void AnimationManager::UpdateTank(const std::string& name, const ObjRectangle& rect, const Direction& dir)
{
	for (auto& object: _tankObjects)
	{
		if (object.name == name)
		{
			//Update tank animation position and dir
			object.rect.x = rect.x;
			object.rect.y = rect.y;
			object.dir = dir;

			UpdateFrame(object, 2);
			return;
		}
	}
}

void AnimationManager::DisableTankAnimation(const std::string& name)//TODO: add reuse flow for animation
{
	for (auto& object: _tankObjects)
	{
		if (object.name == name)
		{
			object.markToDispose = true;
			return;
		}
	}
}

void AnimationManager::DeleteTankAnimation(const std::string& name)
{
	std::erase_if(_tankObjects, [&name](const auto& object)
	{
		return object.name == name;
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

void AnimationManager::DrawObject(const AnimatedObject& object) const
{
	const int currenAnimationFrame = {object.type == AnimationType::Water_Animation
										  ? -object.animationFrame//TODO: -animationFrame -> +animationFrame 
										  : object.animationFrame};//TODO: move this logic to UpdateFrameInfinite
	_events->EmitEvent("DrawAnimation", object.rect, object.dir, currenAnimationFrame, object.scale, object.name);
}

void AnimationManager::Draw() const
{
	for (auto& object: _waterObjects)
	{
		if (object.markToDispose)
		{
			continue;
		}

		DrawObject(object);
	}

	for (auto& object: _tankObjects)
	{
		if (object.markToDispose)
		{
			continue;
		}

		DrawObject(object);
	}

	for (auto& object: _animatedObjects)
	{
		if (object.markToDispose)
		{
			continue;
		}

		DrawObject(object);
	}
}

//TODO: add reuse flow for explosions like bullet pool
