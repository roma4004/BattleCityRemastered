#include "../../headers/bonuses/BonusSpawner.h"
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

BonusSpawner::BonusSpawner(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                           std::shared_ptr<int[]> windowBuffer, UPoint windowSize, const int sideBarWidth,
                           const int bonusSize)
	: _name{"BonusSpawner"},
	  _events{std::move(events)},
	  _windowSize{std::move(windowSize)},
	  _bonusSize{bonusSize},
	  _windowBuffer{std::move(windowBuffer)},
	  _allObjects{allObjects},
	  _distSpawnPosY{0, static_cast<int>(_windowSize.y) - bonusSize},
	  _distSpawnPosX{0, static_cast<int>(_windowSize.x) - sideBarWidth - bonusSize},
	  _distSpawnType{0, 3},
	  _distRandColor{0, std::numeric_limits<int>::max()},
	  _lastTimeBonusSpawn{std::chrono::system_clock::now()}
{
	std::random_device rd;
	_gen = std::mt19937(
			static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) + rd());

	Subscribe();
}

BonusSpawner::~BonusSpawner()
{
	Unsubscribe();
}

void BonusSpawner::Subscribe()
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

void BonusSpawner::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener<const float>("TickUpdate", _name);
}

void BonusSpawner::TickUpdate(const float /*deltaTime*/)
{
	if (TimeUtils::IsCooldownFinish(_lastTimeBonusSpawn, _cooldownBonusSpawn))
	{
		bool isFreeSpawnSpot = true;
		const auto size = static_cast<float>(_bonusSize);
		const auto x = static_cast<float>(_distSpawnPosX(_gen));
		const auto y = static_cast<float>(_distSpawnPosY(_gen));
		const ObjRectangle rect{.x = x, .y = y, .w = size, .h = size};
		for (const std::shared_ptr<BaseObj>& object: *_allObjects)
		{
			if (ColliderUtils::IsCollide(rect, object->GetShape()))
			{
				isFreeSpawnSpot = false;
				break;
			}//TODO: use std::any_of algorithm
		}

		if (isFreeSpawnSpot)
		{
			SpawnRandomBonus(rect);
		}
	}
}

void BonusSpawner::SpawnBonus(const ObjRectangle rect, const int color, const int bonusId)
{
	switch (bonusId)
	{
		case 0:
			SpawnBonus<BonusTimer>(rect, color);
			break;
		case 1:
			SpawnBonus<BonusHelmet>(rect, color);
			break;
		case 2:
			SpawnBonus<BonusGrenade>(rect, color);
			break;
		case 3:
			SpawnBonus<BonusTank>(rect, color);
			break;
		case 4:
			SpawnBonus<BonusStar>(rect, color);
			break;
		case 5:
			SpawnBonus<BonusShovel>(rect, color);
			break;
		default:
			break;
	}
}

void BonusSpawner::SpawnRandomBonus(const ObjRectangle rect)
{
	const int color = _distRandColor(_gen);
	const int bonusId = _distSpawnType(_gen);
	SpawnBonus(rect, color, bonusId);

	_lastTimeBonusSpawn = std::chrono::system_clock::now();
}

template<typename TBonusType>
void BonusSpawner::SpawnBonus(const ObjRectangle rect, const int color) {
	constexpr int bonusLifetimeMs = 10 * 1000;// 10 sec
	constexpr int bonusDurationTimeMs = 10 * 1000;// 10 sec

	_allObjects->emplace_back(std::make_shared<TBonusType>(rect, _windowBuffer, _windowSize, _events,
														   bonusDurationTimeMs, bonusLifetimeMs, color));
}
