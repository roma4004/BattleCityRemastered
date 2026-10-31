#pragma once
#include "components/EventSystem.h"
#include "entities/EffectTarget.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include "utils/Timer.h"
#include <array>
#include <chrono>
#include <concepts>
#include <memory>
#include <optional>
#include <string>
#include <vector>

struct BonusEffectsChangedEvent;
struct GameModeAppliedEvent;
struct GameResetEvent;
struct MapLoadedEvent;
struct RespawnCountChangedToEvent;
struct SeatHoldersAnnouncedEvent;
struct SideBarEnemyBadge;
struct SideBarSeat;
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
		std::chrono::milliseconds duration{};
	};

	std::vector<RunningEffect> _effects{};

	struct EnemyBadge final
	{
		Author seat{};
		BonusType type{};
		int cell{};
	};

	//NOTE: a badge keeps its cell until it runs out - the reserve shrinking above it does not move it
	std::vector<EnemyBadge> _enemyBadges{};
	const GameConfig& _gameConfig;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Subscribe();
	void PlaceEnemyBadges();
	[[nodiscard]] std::vector<SideBarEnemyBadge> EnemyBadges() const;
	[[nodiscard]] std::optional<float> EnemyBadgeSpent(Author seat, BonusType type) const;
	[[nodiscard]] std::vector<SideBarSeat> Seats() const;
	[[nodiscard]] unsigned short LivesOf(PlayerSlot slot) const;
	[[nodiscard]] unsigned short RespawnCountOf(RespawnGroup group) const;
	[[nodiscard]] std::optional<float> SpentOn(const std::predicate<const RunningEffect&> auto& isIt) const;
	[[nodiscard]] std::optional<float> HelmetSpent(Author seat) const;
	[[nodiscard]] static float Spent(const RunningEffect& effect);
	[[nodiscard]] static bool IsRunning(const RunningEffect& effect);
	[[nodiscard]] static bool FreezesEnemyTeam(const RunningEffect& effect);
	[[nodiscard]] static bool FortifiesBase(const RunningEffect& effect);
	[[nodiscard]] std::string LabelOf(PlayerSlot slot) const;
	void OnGameModeApplied(const GameModeAppliedEvent& event);
	void OnGameReset(const GameResetEvent&);
	void OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event);
	void OnMapLoaded(const MapLoadedEvent& event);
	void OnSeatHoldersAnnounced(const SeatHoldersAnnouncedEvent& event);
	void OnBonusEffectsChanged(const BonusEffectsChangedEvent& event);

public:
	RightSideBar(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	void Draw() const;
};
