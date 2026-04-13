#include "components/managers/DelayedSpawnManager.h"
#include "components/EventSystem.h"
#include "entities/pawns/Tank.h"
#include "utils/Timer.h"

DelayedSpawnManager::DelayedSpawnManager(const std::shared_ptr<EventSystem>& events)
	: _name{"DelayedSpawnManager"}
	, _events{events}
{
	Subscribe();
}

DelayedSpawnManager::~DelayedSpawnManager()
{
	Unsubscribe();
}

using milliseconds = std::chrono::milliseconds;

void DelayedSpawnManager::Subscribe()
{
	_events->AddListener("Reset", _name, [this]() { Reset(); });

	_events->AddListener(
			"SpawnDelayStart", _name,
			[this](const std::shared_ptr<Tank>& tank, const milliseconds delay)
			{
				if (delay == milliseconds(0))
				{
					_events->EmitEvent("SpawnEnabled", std::weak_ptr<Tank>(tank));
					// NOTE: immediate call, for tests
				}
				else
				{
					this->_spawnDelays.emplace_back(tank, Timer{delay, std::chrono::system_clock::now()});
				}
			});

	_events->AddListener("PreTickUpdate", _name, [this](const double deltaTime) { this->PreTickUpdate(deltaTime); });

	_events->AddListener("PostTickUpdate", _name, [this](const double /*deltaTime*/) { this->Disposer(); });
}

void DelayedSpawnManager::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("SpawnDelayStart", _name);
	_events->RemoveListener("PreTickUpdate", _name);
	_events->RemoveListener("PostTickUpdate", _name);
}

void DelayedSpawnManager::Reset()
{
	_spawnDelays.clear();
}

void DelayedSpawnManager::PreTickUpdate(const double /*deltaTime*/)
{
	for (auto& [tank, timer]: _spawnDelays)
	{
		if (timer.isActive && timer.IsCooldownFinish())
		{
			_events->EmitEvent("SpawnEnabled", std::weak_ptr<Tank>(tank));
			timer.isActive = false;
		}
	}
}

void DelayedSpawnManager::Disposer()
{
	std::erase_if(_spawnDelays, [](const SpawnDelay& delay)
	{
		return delay.timer.isActive == false;
	});
}
