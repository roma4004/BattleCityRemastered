#pragma once

#include "components/EventSystem.h"
#include "enums/RespawnGroup.h"
#include <boost/uuid/uuid.hpp>

enum class TankType : char8_t;
enum class GameMode : char8_t;
class EventSystem;
struct GameResetEvent;
struct GameModeChangedToEvent;
struct TankSpawnEvent;
struct TankDiedEvent;
struct BonusTankPickupEvent;
struct PlayersBaseFinishedEvent;
struct RespawnTanksEvent;
struct ClientInBonusTankPickupEvent;
struct ClientInRespawnTankEvent;

class RespawnManager final
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime on every GameModeChangedToEvent, independent of _subs's fixed
	// subscribe-once-at-construction lifetime - clearing this vector auto-unsubscribes just this group.
	std::vector<EventSubscription> _clientSubs{};

	// TODO: use std::atomic when multithreading is used
	std::vector<unsigned short> _respawnCount{20u, 3u, 3u};

	struct SpawnSlot
	{
		buuid uuid{};
		TankType type{};
		RespawnGroup group{};
		bool isAvailable{};
	};

	GameMode _gameMode{};
	unsigned short _enemiesSpawnCount{};
	unsigned short _enemiesDeathCount{};
	unsigned short _playersSpawnCount{};
	unsigned short _playersDeathCount{};

	void OnBonusTank(const std::string& author);
	void OnClientInBonusTankPickup(const ClientInBonusTankPickupEvent& event);
	void OnClientRespawn(const ClientInRespawnTankEvent& event);

	void Subscribe();
	void OnGameReset(const GameResetEvent&);
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);
	void OnBonusTankPickup(const BonusTankPickupEvent& event);
	void OnPlayersBaseFinished(const PlayersBaseFinishedEvent&);
	void OnRespawnTanks(const RespawnTanksEvent& event);
	void SubscribeAsClient();

	void UnsubscribeAsClient();

	void SetEnemyNeedRespawn();
	void SetPlayerNeedRespawn();

	void ResetRespawnStat();
	void ResetSpawn();
	void OnGameModeChange();

	[[nodiscard]] static std::string RespawnCountEnumToString(RespawnGroup type);
	void ChangeRespawnCount(int delta, RespawnGroup type);
	void TriggerLastPlayersLife();

	void OnTankSpawn(const TankSpawnEvent& event);
	[[nodiscard]] static bool IsEnemyGroup(RespawnGroup group);
	void OnEnemyDied(bool isAvailable);
	void OnPlayerDied(bool isAvailable);
	void OnTankDied(const TankDiedEvent& event);
	void RespawnTanks(bool skipDelay);

public:
	std::vector<SpawnSlot> _slots{};

	explicit RespawnManager(const std::shared_ptr<EventSystem>& events);

	~RespawnManager() = default;
};
