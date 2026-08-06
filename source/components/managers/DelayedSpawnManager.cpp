#include "components/managers/DelayedSpawnManager.h"
#include "components/EventSystem.h"
#include "components/SpawnEvents.h"
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

void DelayedSpawnManager::Subscribe()
{
	_events->AddListener("Reset", _name, [this]() { this->Reset(); });

	_events->AddListener("SpawnDelayStart", _name, [this](const SpawnDelayStartEvent& event)
	{
		this->SpawnDelayStart(event.obj, event.delay);
	});

	_events->AddListener("PreTickUpdate", _name, [this](const double deltaTime) { this->PreTickUpdate(deltaTime); });

	_events->AddListener("PostTickUpdate", _name, [this](const double /*deltaTime*/) { this->Disposer(); });
}

void DelayedSpawnManager::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void DelayedSpawnManager::Reset()
{
	_spawnDelays.clear();
}

void DelayedSpawnManager::PreTickUpdate(const double /*deltaTime*/)
{
	for (auto& [obj, timer]: _spawnDelays)
	{
		if (timer.isActive && timer.IsCooldownFinish())
		{
			if (obj)
			{
				_events->EmitEvent("SpawnEnabled", obj);
			}

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

void DelayedSpawnManager::SpawnDelayStart(const std::shared_ptr<BaseObj>& obj, const milliseconds delay)
{
	if (!obj)
	{
		return;
	}

	if (delay == milliseconds{0})
	{
		// NOTE: immediate call, for unit tests
		_events->EmitEvent("SpawnEnabled", obj);
	}
	else
	{
		_spawnDelays.emplace_back(obj, Timer{delay, std::chrono::system_clock::now()});
	}
}
