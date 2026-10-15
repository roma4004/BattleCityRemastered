#pragma once
#include "components/EventSystem.h"
#include "entities/EffectTarget.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include "utils/Timer.h"
#include <array>
#include <cstddef>
#include <functional>
#include <memory>
#include <vector>

struct BonusEffectsChangedEvent;
struct GameResetEvent;
struct MapLoadedEvent;
struct RespawnCountChangedToEvent;
struct SeatHoldersAnnouncedEvent;
struct UiCell;
struct UiTable;
class GameConfig;

class RightSideBar final
{
	//NOTE: indexed by RespawnGroup
	std::array<unsigned short, kRespawnGroupCount> _respawnCounts{};
	unsigned short _stageNumber{1u};
	//NOTE: as the host told them - a client in a seat a bot holds is watching it
	std::array<SeatHolder, kSeatCount> _holders{};

	struct RunningEffect final
	{
		BonusType type{};
		EffectTarget target{};
		Author spared{};
		Timer timer{};
		std::size_t reserveCell{};
	};

	std::vector<RunningEffect> _effects{};
	const GameConfig& _gameConfig;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Subscribe();
	[[nodiscard]] UiTable EnemiesTable() const;
	[[nodiscard]] UiTable CountersTable() const;
	//NOTE: empty when none - still holds its column
	[[nodiscard]] UiCell EffectCell(const std::function<bool(const RunningEffect&)>& isIt) const;
	[[nodiscard]] static UiCell FadingCell(const RunningEffect& effect);
	[[nodiscard]] static bool IsRunning(const RunningEffect& effect);
	//NOTE: never a clock an enemy took, even in a free-for-all
	[[nodiscard]] static bool IsOnEnemies(const RunningEffect& effect);
	//NOTE: a helmet takes its tank's cell, a clock the lowest free one in the frame
	[[nodiscard]] std::size_t PickCell(BonusType type) const;
	[[nodiscard]] std::size_t EnemyReserve() const;
	[[nodiscard]] bool IsWatching() const;
	[[nodiscard]] UiTable HintTable() const;
	void OnGameReset(const GameResetEvent&);
	void OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event);
	void OnMapLoaded(const MapLoadedEvent& event);
	void OnSeatHoldersAnnounced(const SeatHoldersAnnouncedEvent& event);
	void OnBonusEffectsChanged(const BonusEffectsChangedEvent& event);

public:
	RightSideBar(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	void Draw() const;
};
