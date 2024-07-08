#include "../headers/BonusSystem.h"
#include "../headers/BonusGrenade.h"
#include "../headers/BonusHelmet.h"
#include "../headers/BonusTimer.h"
#include "../headers/ColliderCheck.h"
#include "../headers/TimeUtils.h"

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
	  _distSpawnType{0, 2},
	  _distRandColor{0, std::numeric_limits<int>::max()}
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

void BonusSystem::TickUpdate(const float deltaTime)
{
	if (TimeUtils::IsCooldownFinish(_lastTimeBonusSpawn, _cooldownBonusSpawn))
	{
		bool isFreeSpawnSpot = true;
		const float size = static_cast<float>(_bonusSize);
		Rectangle rect{static_cast<float>(_distSpawnPosX(_gen)), static_cast<float>(_distSpawnPosY(_gen)), size, size};
		for (const std::shared_ptr<BaseObj>& object: *_allObjects)
		{
			if (ColliderCheck::IsCollide(rect, object->GetShape()))
			{
				isFreeSpawnSpot = false;
				break;
			}
		}

		if (isFreeSpawnSpot)
		{
			const int color = _distRandColor(_gen);
			const int bonusId = _distSpawnType(_gen);
			constexpr int bonusLifetime = 10;
			constexpr int bonusDurationTime = 10;
			if (bonusId == 0)
			{
				_allObjects->emplace_back(std::make_shared<BonusTimer>(rect, _windowBuffer, _windowSize, _events,
				                                                       bonusDurationTime, bonusLifetime, color));
			}
			else if (bonusId == 1)
			{
				_allObjects->emplace_back(std::make_shared<BonusHelmet>(rect, _windowBuffer, _windowSize, _events,
				                                                        bonusDurationTime, bonusLifetime, color));
			}
			else if (bonusId == 2)
			{
				_allObjects->emplace_back(std::make_shared<BonusGrenade>(rect, _windowBuffer, _windowSize, _events,
				                                                         bonusDurationTime, bonusLifetime, color));
			}

			_lastTimeBonusSpawn = std::chrono::system_clock::now();
		}
	}
}
