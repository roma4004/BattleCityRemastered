#include "components/managers/SpawnDelayManager.h"
#include "components/EventSystem.h"
#include "entities/pawns/Tank.h"
#include "utils/Timer.h"

SpawnDelayManager::SpawnDelayManager(const std::shared_ptr<EventSystem>& events)
	: _name{"SpawnDelayManager"}, _events{events}
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

	//TODO: create flow to enable replicated pawn on client after delay end, replicate enable signal
	_events->AddListener("SpawnDelayStart", _name, [this](const std::shared_ptr<Tank>& tank, const milliseconds delay)
	{
		if (delay == milliseconds(0))
		{
			_events->EmitEvent("SpawnEnabled", std::weak_ptr<Tank>(tank));
			// NOTE: immediate call, for tests
		}
		else
		{
			//TODO: fix delay managers to work with on client side
			this->_spawnDelays.emplace_back(tank, Timer{delay, std::chrono::system_clock::now()});
		}
	});

	_events->AddListener("TickUpdate", _name, [this](const float deltaTime)
	{
		this->TickUpdate(deltaTime);
	});

	_events->AddListener("PostTickUpdate", _name, [this](const float /*deltaTime*/) { this->Disposer(); });
}

void SpawnDelayManager::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("SpawnDelayStart", _name);
	_events->RemoveListener("TickUpdate", _name);
	_events->RemoveListener("PostTickUpdate", _name);
}

void SpawnDelayManager::Reset()
{
	_spawnDelays.clear();
}

void SpawnDelayManager::TickUpdate(const float /*deltaTime*/)
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

void SpawnDelayManager::Disposer()
{
	std::erase_if(_spawnDelays, [](const SpawnDelay& delay)
	{
		return delay.timer.isActive == false;
	});
}
