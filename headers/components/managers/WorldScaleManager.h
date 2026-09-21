#pragma once

#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include <memory>
#include <vector>

struct WorldSnapshotRequestedEvent;
class GameConfig;

class WorldScaleManager final
{
public:
	WorldScaleManager(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig);

private:
	void Subscribe();
	void OnMapLoaded(const MapLoadedEvent& event);
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	GameConfig& _gameConfig;
	//NOTE: kept so the snapshot can carry it - a client has no map to read the size off
	MapLoadedEvent _map{};
};
