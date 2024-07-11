#include "../../headers/bonuses/BonusSystem.h"
#include "../../headers/bonuses/BonusGrenade.h"
#include "../../headers/bonuses/BonusHelmet.h"
#include "../../headers/bonuses/BonusShovel.h"
#include "../../headers/bonuses/BonusStar.h"
#include "../../headers/bonuses/BonusTank.h"
#include "../../headers/bonuses/BonusTimer.h"
#include "../../headers/utils/ColliderUtils.h"
#include "../../headers/utils/TimeUtils.h"

#include <chrono>
#include <limits>
#include <memory>

class BaseObj;
class EventSystem;

BonusSystem::BonusSystem(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                         int* windowBuffer, const UPoint windowSize, const int sideBarWidth, const int bonusSize)
	: _name{"BonusSystem"},
	  _events{std::move(events)},
	  _windowSize{windowSize},
	  _bonusSize{bonusSize},
	  _windowBuffer{windowBuffer},
	  _allObjects{allObjects},
	  _distSpawnPosY{0, static_cast<int>(_windowSize.y) - bonusSize},
	  _distSpawnPosX{0, static_cast<int>(_windowSize.x) - sideBarWidth - bonusSize},
	  _distSpawnType{0, 3},
	  _distRandColor{0, std::numeric_limits<int>::max()},
	  _lastTimeBonusSpawn{std::chrono::system_clock::now()}
{
	std::random_device rd;
	_gen = std::mt19937(std::chrono::high_resolution_clock::now().time_since_epoch().count() + rd());

	Subscribe();
}

BonusSystem::~BonusSystem()
{
	Unsubscribe();
};

void BonusSystem::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener<const float>("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});
}

void BonusSystem::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);
}

void BonusSystem::TickUpdate(const float /*deltaTime*/)
{
	if (TimeUtils::IsCooldownFinish(_lastTimeBonusSpawn, _cooldownBonusSpawn))
	{
		bool isFreeSpawnSpot = true;
		const float size = static_cast<float>(_bonusSize);
		const float x = static_cast<float>(_distSpawnPosX(_gen));
		const float y = static_cast<float>(_distSpawnPosY(_gen));
		const Rectangle rect{x, y, size, size};
		for (const std::shared_ptr<BaseObj>& object: *_allObjects)
		{
			if (ColliderUtils::IsCollide(rect, object->GetShape()))
			{
				isFreeSpawnSpot = false;
				break;
			}
		}

		if (isFreeSpawnSpot)
		{
			SpawnRandomBonus(rect);
		}
	}
}

void BonusSystem::SpawnBonus(const Rectangle rect, const int color, const int bonusId)
{
	constexpr int bonusLifetimeMs = 10 * 1000;// 10 sec
	constexpr int bonusDurationTimeMs = 10 * 1000;// 10 sec
	switch (bonusId)
	{
		case 0:
			_allObjects->emplace_back(std::make_shared<BonusTimer>(rect, _windowBuffer, _windowSize, _events,
			                                                       bonusDurationTimeMs, bonusLifetimeMs, color));
			break;
		case 1:
			_allObjects->emplace_back(std::make_shared<BonusHelmet>(rect, _windowBuffer, _windowSize, _events,
			                                                        bonusDurationTimeMs, bonusLifetimeMs, color));
			break;
		case 2:
			_allObjects->emplace_back(std::make_shared<BonusGrenade>(rect, _windowBuffer, _windowSize, _events,
			                                                         bonusDurationTimeMs, bonusLifetimeMs, color));
			break;
		case 3:
			_allObjects->emplace_back(std::make_shared<BonusTank>(rect, _windowBuffer, _windowSize, _events,
			                                                      bonusDurationTimeMs, bonusLifetimeMs, color));
			break;
		case 4:
			_allObjects->emplace_back(std::make_shared<BonusStar>(rect, _windowBuffer, _windowSize, _events,
			                                                      bonusDurationTimeMs, bonusLifetimeMs, color));
			break;
		case 5:
			_allObjects->emplace_back(std::make_shared<BonusShovel>(rect, _windowBuffer, _windowSize, _events,
			                                                        bonusDurationTimeMs, bonusLifetimeMs, color));
			break;
		default:
			break;
	}
}

void BonusSystem::SpawnRandomBonus(const Rectangle rect)
{
	const int color = _distRandColor(_gen);
	const int bonusId = _distSpawnType(_gen);
	SpawnBonus(rect, color, bonusId);

	_lastTimeBonusSpawn = std::chrono::system_clock::now();
}
