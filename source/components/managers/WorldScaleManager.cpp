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
	_cols = event.cols;
	_rows = event.rows;

	Refit();
}

void WorldScaleManager::OnWindowSizeChangedTo(const WindowSizeChangedToEvent& event)
{
	_gameConfig.windowSize = event.newSize;

	Refit();
}

void WorldScaleManager::Refit()
{
	//NOTE: no map yet means nothing to fit to - a resize before the first level is just a new window size
	if (_cols == 0u || _rows == 0u)
	{
		return;
	}

	const float previousCellSize = _gameConfig.gridOffset;
	const WorldGeometry geometry = WorldGeometry::FitMap(_gameConfig.windowSize, _cols, _rows);

	_gameConfig.ApplyGeometry(geometry, _rows);

	_events->EmitEvent(ScaleFactorChangedToEvent{.scale = _gameConfig.scaleFactor});
	_events->EmitEvent(WorldGeometryChangedEvent{.cellSize = geometry.cellSize,
												 .previousCellSize = previousCellSize});
}
