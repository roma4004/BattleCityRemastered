#pragma once

#include "components/EventSystem.h"

class GameConfig;
struct MapLoadedEvent;

class WorldScaleManager final
{
public:
	WorldScaleManager(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig);

	~WorldScaleManager() = default;

private:
	void Subscribe();
	void OnMapLoaded(const MapLoadedEvent& event);

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	GameConfig& _gameConfig;
};
