#include "components/managers/DelayedSpawnManager.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/pawns/Tank.h"
#include "utils/Timer.h"

DelayedSpawnManager::DelayedSpawnManager(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	Subscribe();
}

void DelayedSpawnManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &DelayedSpawnManager::OnGameReset));
	_subs.push_back(_events->AddListener(this, &DelayedSpawnManager::OnSpawnDelayStart));
	_subs.push_back(_events->AddListener(this, &DelayedSpawnManager::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &DelayedSpawnManager::OnPreTickUpdate));
	_subs.push_back(_events->AddListener(this, &DelayedSpawnManager::OnPostTickUpdate));
}

void DelayedSpawnManager::OnGameReset(const GameResetEvent&) { Reset(); }

void DelayedSpawnManager::OnSpawnDelayStart(const SpawnDelayStartEvent& event)
{
	SpawnDelayStart(event.uuid, event.delay);
}

void DelayedSpawnManager::OnGameModeChangedTo(const GameModeChangedToEvent& event) { _gameMode = event.mode; }

void DelayedSpawnManager::OnPreTickUpdate(const PreTickUpdateEvent& event) { PreTickUpdate(event.deltaTime); }

void DelayedSpawnManager::OnPostTickUpdate(const PostTickUpdateEvent&) { Disposer(); }

void DelayedSpawnManager::Reset()
{
	_spawnDelays.clear();
}

void DelayedSpawnManager::PreTickUpdate(const double /*deltaTime*/)
{
	// Doesn't tick on the client - it materializes only via TankSpawnComplete from the host.
	if (_gameMode == GameMode::PlayAsClient)
	{
		return;
	}

	for (auto& [uuid, timer]: _spawnDelays)
	{
		if (timer.isActive && timer.IsCooldownFinish())
		{
			_events->EmitEvent(TankSpawnDelayFinishedEvent{.uuid = uuid});

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
		_events->EmitEvent(TankSpawnDelayFinishedEvent{.uuid = uuid});
	}
	else
	{
		_spawnDelays.emplace_back(uuid, Timer{delay, std::chrono::system_clock::now()});
	}
}
