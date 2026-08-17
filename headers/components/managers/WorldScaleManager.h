#pragma once

#include "Point.h"
#include "components/EventSystem.h"

class GameConfig;
struct MapLoadedEvent;
struct WindowSizeChangedToEvent;

//NOTE: owns the answer to "how big is a cell right now". Two things can change it - a map of a
//different size and a resized window - and both land here instead of in whoever noticed them.
class WorldScaleManager final
{
public:
	WorldScaleManager(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig);

	~WorldScaleManager() = default;

private:
	void Subscribe();
	void OnMapLoaded(const MapLoadedEvent& event);
	void OnWindowSizeChangedTo(const WindowSizeChangedToEvent& event);

	void Refit();

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	GameConfig& _gameConfig;

	//NOTE: kept so a resize can be refitted against the map already on the field
	std::size_t _cols{};
	std::size_t _rows{};
};
