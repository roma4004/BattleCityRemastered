#include "components/managers/WorldScaleManager.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/WorldGeometry.h"
#include "components/events/CoreLifecycleEvents.h"

WorldScaleManager::WorldScaleManager(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig)
	: _events{events}
	, _gameConfig{gameConfig}
{
	Subscribe();
}

void WorldScaleManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &WorldScaleManager::OnMapLoaded));
	_subs.push_back(_events->AddListener(this, &WorldScaleManager::OnWindowSizeChangedTo));
}

void WorldScaleManager::OnMapLoaded(const MapLoadedEvent& event)
{
	_gameConfig.battlefieldSize = WorldGeometry::ForMap(event.cols, event.rows);

	_events->EmitEvent(WorldGeometryChangedEvent{});
}

void WorldScaleManager::OnWindowSizeChangedTo(const WindowSizeChangedToEvent& event)
{
	_gameConfig.windowSize = event.newSize;
}
