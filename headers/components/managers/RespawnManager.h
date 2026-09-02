#pragma once

#include "components/EventSystem.h"
#include "enums/Author.h"
#include "enums/RespawnGroup.h"
#include "utils/Uuid.h"
#include <chrono>
#include <memory>
#include <string>
#include <vector>

enum class TankType : char8_t;
enum class GameMode : char8_t;
class EventSystem;
struct GameResetEvent;
struct TankSpawnEvent;
struct TankDiedEvent;
struct BonusTankPickupEvent;
struct PlayersBaseFinishedEvent;
struct RespawnTanksEvent;
struct BonusTankAppliedEvent;
struct TankRespawnedEvent;

class RespawnManager final
{
	using milliseconds = std::chrono::milliseconds;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	std::vector<unsigned short> _respawnCount{20u, 3u, 3u};

	struct SpawnSlot
	{
		Uuid uuid{};
		TankType type{};
		RespawnGroup group{};
		bool isAvailable{};
	};

	GameMode _gameMode{};
	unsigned short _enemiesSpawnCount{};
	unsigned short _enemiesDeathCount{};
	unsigned short _playersSpawnCount{};
	unsigned short _playersDeathCount{};

	void OnBonusTank(Author author);
	void OnBonusTankApplied(const BonusTankAppliedEvent& event);
	void OnTankRespawned(const TankRespawnedEvent& event);

	void Subscribe();
	void OnGameReset(const GameResetEvent&);
	void OnBonusTankPickup(const BonusTankPickupEvent& event);
	void OnPlayersBaseFinished(const PlayersBaseFinishedEvent&);
	void OnRespawnTanks(const RespawnTanksEvent& event);

	void SetEnemyNeedRespawn();
	void SetPlayerNeedRespawn();

	void ResetRespawnStat();
	void ResetSpawn();

	[[nodiscard]] static std::string RespawnCountEnumToString(RespawnGroup type);
	void ChangeRespawnCount(int delta, RespawnGroup type);
	void TriggerLastPlayersLife();

	void OnTankSpawn(const TankSpawnEvent& event);
	[[nodiscard]] static bool IsEnemyGroup(RespawnGroup group);
	void OnEnemyDied(bool isAvailable);
	void OnPlayerDied(bool isAvailable);
	void OnTankDied(const TankDiedEvent& event);
	void RespawnTanks();

public:
	std::vector<SpawnSlot> _slots{};

	RespawnManager(const std::shared_ptr<EventSystem>& events, GameMode gameMode);

	~RespawnManager() = default;
};
