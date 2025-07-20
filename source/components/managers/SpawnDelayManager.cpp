#include "components/managers/SpawnDelayManager.h"
#include "components/EventSystem.h"
#include "entities/pawns/Tank.h"
#include "utils/Timer.h"

SpawnDelayManager::SpawnDelayManager(std::shared_ptr<EventSystem> events)
	: _name{"SpawnDelayManager"}, _events{std::move(events)}
{
	Subscribe();
}

SpawnDelayManager::~SpawnDelayManager()
{
	Unsubscribe();
}

using milliseconds = std::chrono::milliseconds;
void SpawnDelayManager::Subscribe()
{
	_events->AddListener("Reset", _name, [this]() { Reset(); });

	_events->AddListener("SpawnDelayStart", _name, [this](std::shared_ptr<Tank> tank, const milliseconds delay)
	{
		if (delay == milliseconds(0))
		{
			_events->EmitEvent("SpawnEnabled", std::weak_ptr<Tank>(tank)); // NOTE: immediate call, for tests
		}
		else
		{
			this->_spawnDelays.emplace_back(tank, Timer{delay, std::chrono::system_clock::now()});
		}
	});

	_events->AddListener("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});

	_events->AddListener("DisposeStage", _name, [this]() { this->Disposer(); });
}

void SpawnDelayManager::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("SpawnDelayStart", _name);
	_events->RemoveListener("TickUpdate", _name);
	_events->RemoveListener("DisposeStage", _name);
}

void SpawnDelayManager::Reset()
{
	_spawnDelays.clear();
}

void SpawnDelayManager::TickUpdate(const float /*deltaTime*/)
{
	for (size_t i = 0u; i < _spawnDelays.size(); ++i)
	{
		if (auto& [tank, timer] = _spawnDelays[i];
			timer.isActive && timer.IsCooldownFinish())
		{
			tank->Enable();
			_events->EmitEvent("SpawnEnabled", std::weak_ptr<Tank>(tank));
			timer.isActive = false;
		}
	}
}

void SpawnDelayManager::Disposer()
{
	std::erase_if(_spawnDelays, [](const SpawnDelay& delay)
	{
		return delay.timer.isActive == false;
	});
}
