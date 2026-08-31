#pragma once

#include "components/EventSystem.h"
#include "geometry/ObjRectangle.h"
#include <memory>
#include <vector>

enum class ObstacleType : char8_t;
class BaseObj;
class EventSystem;
struct FortressSpotRegisteredEvent;
struct BonusShovelStatusChangeEvent;
struct GameResetEvent;

// Keeps the places the eagle's wall stands in, filled once from the map. A spot outlives the wall in
// it - that is what lets the shovel rebuild one shot out earlier, when no object is left to ask.
class FortressManager final
{
	struct Spot
	{
		ObjRectangle rect;
		std::weak_ptr<BaseObj> wall;
	};

	const std::vector<std::shared_ptr<BaseObj>>& _allObjects;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	std::vector<Spot> _spots{};

	void Subscribe();
	void OnSpotRegistered(const FortressSpotRegisteredEvent& event);
	void OnBonusShovel(const BonusShovelStatusChangeEvent& event);
	void OnGameReset(const GameResetEvent&);

	void ClearSpot(const Spot& spot) const;
	void Rebuild(const Spot& spot, ObstacleType material) const;

public:
	FortressManager(const std::shared_ptr<EventSystem>& events, const std::vector<std::shared_ptr<BaseObj>>& allObjects);

	~FortressManager();
};
