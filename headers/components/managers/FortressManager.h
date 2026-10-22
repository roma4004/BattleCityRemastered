#pragma once

#include "components/EventSystem.h"
#include "geometry/ObjRectangle.h"
#include <memory>
#include <vector>

enum class ObstacleType : char8_t;
struct FortressSpotRegisteredEvent;
struct BonusShovelStatusChangeEvent;
struct GameResetEvent;
class BaseObj;
class EventSystem;
class GameConfig;

//NOTE: the eagle wall's spots from the map; a spot outlives its wall, so the shovel can rebuild a destroyed one
class FortressManager final
{
	struct Spot
	{
		ObjRectangle rect;
		std::weak_ptr<BaseObj> wall;
	};

	const std::vector<std::shared_ptr<BaseObj>>& _allObjects;
	const GameConfig& _gameConfig;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	std::vector<Spot> _spots{};

	void Subscribe();
	void OnSpotRegistered(const FortressSpotRegisteredEvent& event);
	void OnBonusShovel(const BonusShovelStatusChangeEvent& event) const;
	void OnGameReset(const GameResetEvent&);

	void ClearSpot(const Spot& spot) const;
	void Rebuild(const Spot& spot, ObstacleType material) const;

public:
	FortressManager(const std::shared_ptr<EventSystem>& events,
					const std::vector<std::shared_ptr<BaseObj>>& allObjects, const GameConfig& gameConfig);
};
