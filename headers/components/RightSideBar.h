#pragma once
#include "components/EventSystem.h"
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include <array>
#include <memory>
#include <vector>

struct MapLoadedEvent;
struct RespawnCountChangedToEvent;
struct SeatHoldersAnnouncedEvent;
struct UiTable;
class GameConfig;

class RightSideBar final
{
	//NOTE: indexed by RespawnGroup
	std::array<unsigned short, kRespawnGroupCount> _respawnCounts{};
	unsigned short _stageNumber{1u};
	//NOTE: as the host told them - a client in a seat a bot holds is watching it
	std::array<SeatHolder, kSeatCount> _holders{};
	const GameConfig& _gameConfig;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Subscribe();
	[[nodiscard]] UiTable EnemiesTable() const;
	[[nodiscard]] UiTable CountersTable() const;
	[[nodiscard]] bool IsWatching() const;
	[[nodiscard]] UiTable HintTable() const;
	void OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event);
	void OnMapLoaded(const MapLoadedEvent& event);
	void OnSeatHoldersAnnounced(const SeatHoldersAnnouncedEvent& event);

public:
	RightSideBar(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	void Draw() const;
};
