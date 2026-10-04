#pragma once

#include "components/EventSystem.h"
#include "enums/Author.h"
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include "utils/Uuid.h"
#include <array>
#include <chrono>
#include <cstddef>
#include <memory>
#include <vector>

enum class TankType : char8_t;
enum class GameMode : char8_t;
class GameConfig;
struct GameResetEvent;
struct EnemyLineupLoadedEvent;
struct TankSpawnEvent;
struct TankDiedEvent;
struct BonusTankPickupEvent;
struct PlayersBaseFinishedEvent;
struct RespawnTanksEvent;
struct BonusTankAppliedEvent;
struct TankRespawnedEvent;
struct SeatsFilledEvent;
struct SeatHolderChangedEvent;
struct WorldSnapshotRequestedEvent;
struct WorldSnapshotReceivedEvent;
class EventSystem;

class RespawnManager final
{
	using milliseconds = std::chrono::milliseconds;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	//NOTE: indexed by RespawnGroup
	std::array<unsigned short, kRespawnGroupCount> _respawnCount{};

	struct SpawnSlot
	{
		Uuid uuid{};
		TankType type{};
		RespawnGroup group{};
		bool isAvailable{};
		bool isOnField{};
	};

	GameMode _gameMode{};
	bool _isFreeForAll{};
	std::size_t _seatCount{};
	//NOTE: an empty seat spawns nothing - a network match says who sits where as it starts
	std::array<SeatHolder, kSeatCount> _holders{};
	std::size_t _enemySeats{};
	unsigned short _enemiesSpawnCount{};
	unsigned short _enemiesDeathCount{};
	unsigned short _playersSpawnCount{};
	unsigned short _playersDeathCount{};

	void OnBonusTank(Author author);
	void OnBonusTankApplied(const BonusTankAppliedEvent& event);
	void OnTankRespawned(const TankRespawnedEvent& event);
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;
	void OnWorldSnapshotReceived(const WorldSnapshotReceivedEvent& event);
	void OnSeatsFilled(const SeatsFilledEvent& event);
	void OnSeatHolderChanged(const SeatHolderChangedEvent& event);
	void Vacate(SpawnSlot& seat);

	void Subscribe();
	void OnGameReset(const GameResetEvent& event);
	void OnBonusTankPickup(const BonusTankPickupEvent& event);
	void OnPlayersBaseFinished(const PlayersBaseFinishedEvent&);
	void OnRespawnTanks(const RespawnTanksEvent& event);
	void OnEnemyLineupLoaded(const EnemyLineupLoadedEvent& event);

	void SetEnemyNeedRespawn();
	void SetPlayerNeedRespawn();

	void ResetRespawnStat(bool keepsPlayerLives);
	void ResetSpawn(bool keepsPlayerLives = false);

	void ChangeRespawnCount(int delta, RespawnGroup type);
	void TriggerLastPlayersLife();

	void OnTankSpawn(const TankSpawnEvent& event);
	void OnEnemyDied(bool isAvailable);
	void OnPlayerDied(bool isAvailable);
	[[nodiscard]] bool AreEnemiesGone() const;
	[[nodiscard]] std::size_t PlayersStillIn() const;
	void OnTankDied(const TankDiedEvent& event);
	void RespawnTanks();

	std::vector<SpawnSlot> _slots{};

public:
	RespawnManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);
};
