#pragma once
#include "components/EventSystem.h"
#include "enums/RespawnGroup.h"
#include <array>
#include <memory>
#include <vector>

struct DrawUserInterfaceEvent;
struct MapLoadedEvent;
struct RespawnCountChangedToEvent;
struct UiTable;
class GameConfig;

class RightSideBar final
{
	//NOTE: indexed by RespawnGroup
	std::array<unsigned short, kRespawnGroupCount> _respawnCounts{};
	unsigned short _stageNumber{1u};
	const GameConfig& _gameConfig;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Subscribe();
	void Draw() const;
	[[nodiscard]] UiTable EnemiesTable() const;
	[[nodiscard]] UiTable CountersTable() const;
	void OnDrawUserInterface(const DrawUserInterfaceEvent&) const;
	void OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event);
	void OnMapLoaded(const MapLoadedEvent& event);

public:
	RightSideBar(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);
};
