#include "components/managers/DelayedSpawnManager.h"
#include "components/EventSystem.h"
#include "components/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/TimingEvents.h"
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
	_events->AddListener(_name, [this](const GameResetEvent&) { this->Reset(); });

	_events->AddListener(_name, [this](const SpawnDelayStartEvent& event)
	{
		this->SpawnDelayStart(event.uuid, event.delay);
	});

	_events->AddListener(_name, [this](const PreTickUpdateEvent& event) { this->PreTickUpdate(event.deltaTime); });

	_events->AddListener(_name, [this](const PostTickUpdateEvent&) { this->Disposer(); });
}

void DelayedSpawnManager::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void DelayedSpawnManager::Reset()
{
	_spawnDelays.clear();
}

void DelayedSpawnManager::PreTickUpdate(const double /*deltaTime*/)
{
	for (auto& [uuid, timer]: _spawnDelays)
	{
		if (timer.isActive && timer.IsCooldownFinish())
		{
			_events->EmitEvent(SpawnEnabledEvent{.uuid = uuid});

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

void DelayedSpawnManager::SpawnDelayStart(const buuid& uuid, const milliseconds delay)
{
	if (delay == milliseconds{0})
	{
		// NOTE: immediate call, for unit tests
		_events->EmitEvent(SpawnEnabledEvent{.uuid = uuid});
	}
	else
	{
		_spawnDelays.emplace_back(uuid, Timer{delay, std::chrono::system_clock::now()});
	}
}
