#pragma once

#include "components/EventSystem.h"
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include "utils/Uuid.h"
#include <array>
#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

enum class TankType : char8_t;
struct GameResetEvent;
struct NextLevelEvent;
struct EnemyLineupLoadedEvent;
struct TankSpawnEvent;
struct TankDiedEvent;
struct BonusTankPickupEvent;
struct PlayersBaseFinishedEvent;
struct RespawnTanksEvent;
struct ShareLifeEvent;
struct GamepadButtonEvent;
struct BonusGoldenGrenadePickupEvent;
struct SeatsFilledEvent;
struct SeatHolderChangedEvent;
struct WorldSnapshotRequestedEvent;
struct WorldSnapshotReceivedEvent;
class GameConfig;

class RespawnManager final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	const GameConfig& _gameConfig;

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

	//NOTE: an empty seat spawns nothing - a network match says who sits where as it starts
	std::array<SeatHolder, kSeatCount> _holders{};
	std::size_t _enemySeats{};
	//NOTE: the first of Won and Over stands
	bool _isDecided{};
	std::optional<PlayerSlot> _announcedRecipient{};

	void OnGoldenGrenadePickup(const BonusGoldenGrenadePickupEvent& event);
	void OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const;
	void OnWorldSnapshotReceived(const WorldSnapshotReceivedEvent& event);
	void OnSeatsFilled(const SeatsFilledEvent& event);
	void OnSeatHolderChanged(const SeatHolderChangedEvent& event);
	void Vacate(SpawnSlot& seat);

	void Subscribe();
	void OnGameReset(const GameResetEvent& event);
	void OnNextLevel(const NextLevelEvent&);
	void OnBonusTankPickup(const BonusTankPickupEvent& event);
	void OnPlayersBaseFinished(const PlayersBaseFinishedEvent&);
	void OnRespawnTanks(const RespawnTanksEvent& event);
	void OnEnemyLineupLoaded(const EnemyLineupLoadedEvent& event);
	void OnShareLife(const ShareLifeEvent& event);
	void OnGamepadButton(const GamepadButtonEvent& event);
	[[nodiscard]] bool CanGiveLife(PlayerSlot slot) const;
	[[nodiscard]] std::optional<PlayerSlot> ShareRecipient() const;
	void AnnounceShareRecipient();

	void RecountSeats(RespawnGroup group);

	void ResetRespawnStat(bool keepsPlayerLives);
	void ResetSpawn(bool keepsPlayerLives = false);

	void ChangeRespawnCount(int delta, RespawnGroup group);
	void SetRespawnCount(RespawnGroup group, unsigned short count);

	void OnTankSpawn(const TankSpawnEvent& event);
	void CheckMatchEnd();
	[[nodiscard]] bool AreEnemiesGone() const;
	[[nodiscard]] std::size_t PlayersStillIn() const;
	void OnTankDied(const TankDiedEvent& event);

	std::vector<SpawnSlot> _slots{};

public:
	RespawnManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);
};
